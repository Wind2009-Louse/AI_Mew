#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // random_shuffle
#include "omp.h" // openmp
#include <stdlib.h>// rand
#include <ctime> // rand initialize
#include <fstream>
#include <direct.h> // _mkdir
using namespace std;

#define BOT_PIECE 1
#define PLAYER_PIECE -1
#define EMPTY_PIECE 0

#define BOARD_SIZE 12
#define FINAL_POWER 10000

#define SEARCH_MAX_DEPTH 3 // search depth
#define SEARCH_MAX_DEPTH_END 5 // search depth while at end

const vector<string> full_alpha_list = { "０", "１", "２", "３", "４", "５", "６",
"７", "８", "９", "⑽", "⑾", "⑿"};
const short find_dir[8][2] = { { -1,-1 },{ 0,-1 },{ 1,-1 },
{ -1,0 },{ 1,0 },
{ -1,1 },{ 0,1 },{ 1,1 } };

template<typename T> int find_in_vector(vector<T> v, T f) {
	for (int i = 0; i < v.size(); ++i)
		if (v[i] == f) return i;
	return -1;
}

// get structure according to id and mode(use to define places on board)
// so as to calculate
vector<int> get_indexes(int id, int mode) {
	int begin_x = id % 5;
	int begin_y = id / 5;
	switch (mode / 2)
	{
		case 0:
			break;
		case 1 :
			begin_x = BOARD_SIZE - 1 - begin_x;
			break;
		case 2 :
			begin_y = BOARD_SIZE - 1 - begin_y;
			break;
		case 3 :
			begin_x = BOARD_SIZE - 1 - begin_x;
			begin_y = BOARD_SIZE - 1 - begin_y;
			break;
	}
	int begining = begin_y * BOARD_SIZE + begin_x;
	vector<int> result;
	for (int i = 0; i < 9; ++i) {
		int _a = i % 3;
		int _b = i / 3;
		int real_id;
		/*
		0:+a +b
		1:+b +a
		2:-a +b
		3:+b -a
		4:+a -b
		5:-b +a
		6:-a -b
		7:-b -a
		*/
		switch (mode)
		{
		case 0:
			real_id = begining + _a + (_b * BOARD_SIZE);
			break;
		case 1:
			real_id = begining + _b + (_a * BOARD_SIZE);
			break;
		case 2:
			real_id = begining - _a + (_b * BOARD_SIZE);
			break;
		case 3:
			real_id = begining - _b + (_a * BOARD_SIZE);
			break;
		case 4:
			real_id = begining + _a - (_b * BOARD_SIZE);
			break;
		case 5:
			real_id = begining + _b - (_a * BOARD_SIZE);
			break;
		case 6:
			real_id = begining - _a - (_b * BOARD_SIZE);
			break;
		case 7:
			real_id = begining - _b - (_a * BOARD_SIZE);
			break;
		default:
			break;
		}
		result.push_back(real_id);
	}
	return result;
}

// different places' value
// update in Board
vector< vector<int> > var_lists;

struct Board {
	double alpha;
	double beta;
	short board[BOARD_SIZE * BOARD_SIZE];
	bool is_bot_first;
	bool is_on_bot;
	int total_times;
	int search_depth;

	// initial
	Board(bool bot_first = false) {
		alpha = SHRT_MIN;
		beta = SHRT_MAX;
		is_on_bot = bot_first;
		is_bot_first = bot_first;
		total_times = 0;
		for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) board[i] = EMPTY_PIECE;
		short black = bot_first ? BOT_PIECE : PLAYER_PIECE;
		short white = bot_first ? PLAYER_PIECE : BOT_PIECE;
		// place
		onboard(2, 2) = white;
		onboard(3, 2) = white;
		onboard(4, 2) = white;
		onboard(6, 6) = white;
		onboard(7, 6) = white;
		onboard(8, 6) = white;
		onboard(2, 8) = white;
		onboard(2, 9) = white;
		onboard(9, 2) = black;
		onboard(9, 3) = black;
		onboard(3, 5) = black;
		onboard(4, 5) = black;
		onboard(5, 5) = black;
		onboard(7, 9) = black;
		onboard(8, 9) = black;
		onboard(9, 9) = black;
		search_depth = -1;
		structvar_initial();
	}
	Board(Board* b) {
		alpha = b->alpha;
		beta = b->beta;
		memcpy_s(board, BOARD_SIZE * BOARD_SIZE * sizeof(short), b->board, BOARD_SIZE * BOARD_SIZE * sizeof(short));
		is_on_bot = b->is_on_bot;
		is_bot_first = b->is_bot_first;
		search_depth = b->search_depth;
	}
	void structvar_initial() {
		bool need_rewrite = false;
		for (int i = 0; i < 25; ++i) {
			// add empty vector
			if (var_lists.size() <= i){
				var_lists.push_back(vector<int>());
			}
			var_lists[i].clear();
			// read var from file
			ifstream strvar_file;
			string filename = "struct/";
			filename += to_string(i);
			filename += ".txt";
			strvar_file.open(filename);
			if (strvar_file.is_open()) {
				for (int j = 0; j < pow(3, 9); ++j) {
					int _temp;
					strvar_file >> _temp;
					var_lists[i].push_back(_temp);
				}
				strvar_file.close();
			}
			else {
				// no such file: need to make file afterwards
				strvar_file.close();
				need_rewrite = true;
				for (int j = 0; j < pow(3, 9); ++j) {
					var_lists[i].push_back(0);
				}
			}
		}

		if (need_rewrite) {
			struct_write();
		}
	}
	void struct_write() {
		_mkdir("struct");
		// output
		for (int i = 0; i < 25; ++i) {
			string filename = "struct/";
			filename += to_string(i);
			filename += ".txt";
			ofstream write_file(filename);
			for (int j = 0; j < pow(3, 9); ++j) {
				write_file << var_lists[i][j] << endl;
			}
			write_file.close();
		}
	}
	// x,y's range: 0~5
	short& onboard(int x, int y) {
		if (x < 0 || x > BOARD_SIZE - 1 || y < 0 || y > BOARD_SIZE - 1) {
			cout << "Error!" << endl;
			system("pause");
			throw;
		}
		return board[y * BOARD_SIZE + x];
	}
	vector<int> next_possible() {
		// results: dir * (BOARDSIZE^2) + index
		// decode:
		// dir = result / (BOARDSIZE^2)
		// x = (result % BOARDSIZE^2) % BOARDSIZE
		// y = (result % BOARDSIZE^2) / BOARDSIZE
		vector<int> results;
		for (int _y = 0; _y < BOARD_SIZE; ++_y) {
			for (int _x = 0; _x < BOARD_SIZE; ++_x) {
				int id = _y * BOARD_SIZE + _x;
				// if self's piece
				if (board[id] == (is_on_bot ? BOT_PIECE : PLAYER_PIECE)) {
					// find dir that can move
					for (int i = 0; i < 8; ++i) {
						int nx = _x + find_dir[i][0];
						int ny = _y + find_dir[i][1];
						if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) {
							continue;
						}
						if (onboard(nx, ny) == 0) {
							results.push_back(i * BOARD_SIZE * BOARD_SIZE + id);
						}
					}
				}
			}
		}
		return results;
	}
	void print(bool print_optimal = false) {
		vector<short> optimal;
		//if (print_optimal) optimal = next_possible();
		string bot_piece = (is_bot_first) ? "●" : "○";
		string player_piece = (is_bot_first) ? "○" : "●";
		cout << "　　";
		for (int i = 0; i < BOARD_SIZE; ++i) {
			cout << full_alpha_list[i];
		}
		string block = "";
		for (int i = 0; i < BOARD_SIZE; ++i) {
			block += "━";
		}
		cout << "\n　┏" << block << "┓" << total_times << endl;
		for (int _y = 0; _y < BOARD_SIZE; ++_y) {
			cout << full_alpha_list[_y];
			cout << "┃";
			for (int _x = 0; _x < BOARD_SIZE; ++_x) {
				short this_board = onboard(_x, _y);
				if (this_board == BOT_PIECE) {
					cout << bot_piece;
				}
				else if (this_board == PLAYER_PIECE) {
					cout << player_piece;
				}
				else {
					short this_index = _y * BOARD_SIZE + _x;
					int find_index = find_in_vector(optimal, this_index);
					if (find_index == -1) {
						cout << "　";
					}
					else {
						cout << full_alpha_list[find_index];
					}
				}
			}
			cout << "┃" << endl;
		}
		cout << "　┗" << block << "┛" << endl;
	}
	vector<int> enemy_possible() {
		is_on_bot = !is_on_bot;
		vector<int> enemy_pos = next_possible();
		is_on_bot = !is_on_bot;
		return enemy_pos;
	}
	void new_step(int step_where) {
		short self_piece = (is_on_bot) ? BOT_PIECE : PLAYER_PIECE;
		short enemy_piece = (is_on_bot) ? PLAYER_PIECE : BOT_PIECE;
		int old_id = step_where % (BOARD_SIZE * BOARD_SIZE);
		// decode
		int dir = step_where / (BOARD_SIZE * BOARD_SIZE);
		int nx = (old_id % BOARD_SIZE) + find_dir[dir][0];
		int ny = (old_id / BOARD_SIZE) + find_dir[dir][1];
		// move
		int new_id = ny * BOARD_SIZE + nx;
		board[old_id] = 0;
		board[new_id] = self_piece;
		// squeeze
		for (int i = 0; i < 8; ++i) {
			int _x1 = nx + find_dir[i][0];
			int _y1 = ny + find_dir[i][1];
			int _x2 = _x1 + find_dir[i][0];
			int _y2 = _y1 + find_dir[i][1];
			if (_x1 < 0 || _x1 >= BOARD_SIZE || _y1 < 0 || _y1 >= BOARD_SIZE
				|| _x2 < 0 || _x2 >= BOARD_SIZE || _y2 < 0 || _y2 >= BOARD_SIZE) {
				continue;
			}
			if (onboard(_x1, _y1) == enemy_piece && onboard(_x2, _y2) == self_piece) {
				onboard(_x1, _y1) = self_piece;
			}
		}
		// pick
		for (int i = 0; i < 4; ++i) {
			int _x1 = nx + find_dir[i][0];
			int _y1 = ny + find_dir[i][1];
			int _x2 = nx - find_dir[i][0];
			int _y2 = ny - find_dir[i][1];
			if (_x1 < 0 || _x1 >= BOARD_SIZE || _y1 < 0 || _y1 >= BOARD_SIZE
				|| _x2 < 0 || _x2 >= BOARD_SIZE || _y2 < 0 || _y2 >= BOARD_SIZE) {
				continue;
			}
			if (onboard(_x1, _y1) == enemy_piece && onboard(_x2, _y2) == enemy_piece) {
				onboard(_x1, _y1) = self_piece;
				onboard(_x2, _y2) = self_piece;
			}
		}
		is_on_bot = !is_on_bot;
		total_times++;
		
	}
	short bot_on_ideal_step(bool debug = true, int depth = 0, bool random_move = false) {
		// which search?
		if (depth != 0) {
			search_depth = depth;
		}
		else {
			int empty_count = BOARD_SIZE*BOARD_SIZE;
			for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) {
				if (board[i] != EMPTY_PIECE) empty_count--;
			}
			if (empty_count <= SEARCH_MAX_DEPTH_END) {
				search_depth = SEARCH_MAX_DEPTH_END;
			}
			else {
				search_depth = SEARCH_MAX_DEPTH;
			}
		}
		alpha = DBL_MIN;
		beta = DBL_MAX;
		vector<int> next_steps = next_possible();
		// get move
		pair<int, double> expection = get_self_value(random_move);
		short next_step_id = expection.first;
		if (debug) {
			cout << "电脑认为能够达到的期望值：" << expection.second << endl;
			cout << "电脑下子：(" << (next_step_id % (BOARD_SIZE*BOARD_SIZE) % BOARD_SIZE) << ", " 
									<< (next_step_id % (BOARD_SIZE*BOARD_SIZE) / BOARD_SIZE) << ", "
									<< next_step_id / (BOARD_SIZE*BOARD_SIZE) << ")。" << endl;
		}
		new_step(next_step_id);
		return next_step_id;
	}
	// <move step, value>
	pair<int, double> get_self_value(bool random_move = false) {
		double value = 0;
		vector<int> next_steps = next_possible();
		pair<int, double> result(-1, 0);
		// no place to move
		if (next_steps.size() == 0) {
			return pair<int, double>(-1, (is_on_bot ? -FINAL_POWER : FINAL_POWER));
		}
		if (total_times >= 120) {
			// to leaf
			for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) {
				value += board[i];
			}
			return pair<int, double>(-1, (value<=0 ? -FINAL_POWER : FINAL_POWER));
		}
		// to search bottom
		if (search_depth == 0) {
			double value;
			value = calculate_sturct();
			return pair<int, double>(-1, value);
		}
		if (random_move) {
			random_shuffle(next_steps.begin(), next_steps.end());
		}
		for (int i = 0; i < next_steps.size(); ++i) {
			Board* next_board = new Board(this);
			next_board->new_step(next_steps[i]);
			next_board->search_depth--;
			pair<int, double> next_value = next_board->get_self_value(random_move);
			delete next_board;
			if (
				((result.first == -1) ||
				(result.second < next_value.second && is_on_bot) ||
					(result.second > next_value.second && !is_on_bot)))
			{
				result.first = next_steps[i];
				result.second = next_value.second;
				// alpha update
				if ((alpha < result.second) && is_on_bot) {
					alpha = result.second;
				}
				// beta update
				else if ((beta > result.second) && !is_on_bot) {
					beta = result.second;
				}
			}
			// cut
			if (alpha > beta) {
				//result.first = -2;
				return result;
			}
		}
		return result;
	}
	double calculate_sturct() {
		// end calculate
		if (total_times >= 120) {
			int value = 0;
			for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) {
				value += board[i];
			}
			return value;
		}
		int result = 0;
		// calculate according to each structure
		// use OpenMP to parallelize
#pragma omp parallel for
		for (int i = 0; i < 25; ++i) {
			int _result = 0;
			for (int mode = 0; mode < 8; ++mode) {
				vector<int> list = get_indexes(i, mode);
				int var = 0;
				for (int j = 0; j < list.size(); ++j) {
					var *= 3;
					var += board[list[j]] + 1;
				}
				_result += var_lists[i][var];
			}
#pragma omp atomic
			result += _result;
		}
		return result;
	}
	// reverse all the piece
	void all_reverse() {
		for (int i = 0; i < BOARD_SIZE*BOARD_SIZE; ++i) {
			if (board[i] == BOT_PIECE) board[i] = PLAYER_PIECE;
			else if (board[i] == PLAYER_PIECE) board[i] = BOT_PIECE;
		}
	}
	void update_struct(int power, Board* target = NULL) {
		if (target == NULL) target = this;
#pragma omp parallel for
		for (int i = 0; i < 25; ++i) {
			int _result = 0;
			for (int mode = 0; mode < 8; ++mode) {
				vector<int> list = get_indexes(i, mode);
				int var = 0;
				int _var = 0;
				for (int j = 0; j < list.size(); ++j) {
					var *= 3;
					_var *= 3;
					var += board[list[j]] + 1;
					_var += board[list[j]] * -1 + 1;
				}
				var_lists[i][var] += power;
				var_lists[i][_var] -= power;
			}
		}
	}
};

int pvc() {
	while (true) {
		int mode = -1;
		while (mode == -1) {
			cout << "请选择先后(0=先手, 1=后手):";
			cin >> mode;
			if (mode < 0 || mode > 1) {
				cout << "Illegal input!" << endl;
				if (cin.fail()) {
					cin.sync();
					cin.clear();
					cin.ignore();
				}
				mode = -1;
			}
		}
		int depth;
		cout << "请输入深度：";
		cin >> depth;
		string your = (mode) ? "(白子○)" : "(黑子●)";
		Board* current = new Board(mode == 1);
		vector<Board*> board_records;
		while (true) {
			cout << endl;
			cout << "--------------------------------------" << endl;
			current->print(!current->is_on_bot);
			vector<int> list = current->next_possible();
			// no new step?
			if (list.size() == 0 || current->total_times >= 120) {
				break;
			}
			current->calculate_sturct();
			if (current->is_on_bot) {
				cout << "电脑思考中..." << endl;
				short this_record = current->bot_on_ideal_step(true, depth, true);
			}
			else {
				bool rollback = false;
				string ip;
				int decision;
				while (true) {
					int _x, _y, _dir;
					cout << "轮到你" << your << "下棋，请输入棋子坐标(x,y)和方向，均输入-1则回退。" << endl;
					cout << "012\n3 4\n567" << endl;
					cin >> _x >> _y >> _dir;
					if (_x == -1 && _y == -1 && _dir == -1) {
						if (board_records.size() <= 0) {
							cout << "无法回滚！" << endl;
							continue;
						}
						rollback = true;
					}
					// oversize
					if (_x < 0 || _x >= BOARD_SIZE || _y < 0 || _y >= BOARD_SIZE) {
						cout << "Illgeal!" << endl;
						continue;
					}
					else {
						// legal check
						decision = _dir * (BOARD_SIZE * BOARD_SIZE) + (_y * BOARD_SIZE + _x);
						if (find_in_vector(list, decision) == -1) {
							cout << "Illgeal!" << endl;
							continue;
						}
					}
					break;
				}
				if (rollback) {
					cout << "回滚成功！" << endl;
					delete current;
					current = board_records[board_records.size() - 1];
					board_records.pop_back();
				}
				else {
					// record
					Board* b_copy = new Board(current);
					board_records.push_back(b_copy);
					// move
					current->new_step(decision);
				}
			}
		}
		// calculate
		double value = current->get_self_value(false).second;
		cout << "你的最终得分：" << -value / FINAL_POWER << endl;

		// learn according to the record
		int isbotwin = value < 0 ? 1 : -1;
		for (int i = 0; i < board_records.size(); ++i) {
			current->update_struct(isbotwin, board_records[i]);
			delete board_records[i];
		}
		current->struct_write();

		cout << endl;
		mode = -1;
		while (mode == -1) {
			cout << "是否重新开始？(0=否，1=是)";
			cin >> mode;
			if (mode < 0 || mode > 1) {
				cout << "Illegal!" << endl;
				if (cin.fail()) {
					cin.sync();
					cin.clear();
					cin.ignore();
				}
				mode = -1;
			}
		}
		if (mode == 0) break;
		cout << endl;
	}
	return 0;
}

int struct_train(bool debug = true) {
	int times = 0;
	int depth;
	cout << "请输入查找深度：";
	cin >> depth;
	while (times++<10000) {
		// record
		vector<Board*> records;
		Board* run_board = new Board(true);
		while (true) {
			Board* record = new Board(run_board);
			records.push_back(record);
			if (debug) {
				cout << "--------------------------------------" << endl;
				run_board->print();
				cout << "当前执子：" << ((run_board->is_bot_first ^ run_board->is_on_bot) ? "○" : "●") << endl;
				cout << "当前期望值：" << (run_board->calculate_sturct()) << endl;
			}
			vector<int> current_steps = run_board->next_possible();
			if (current_steps.size() == 0 || run_board->total_times >= 120) {
				break;
			}
			run_board->bot_on_ideal_step(debug, depth, true);
			run_board->all_reverse();
			run_board->is_bot_first = !run_board->is_bot_first;
			run_board->is_on_bot = !run_board->is_on_bot;
		}
		double value = run_board->get_self_value(false).second;
		// true=white, false=black
		bool winner = (run_board->is_bot_first ^ (value > 0));
		for (int i = 0; i < records.size(); ++i) {
			Board* record = records[i];
			// true=white, false=black
			bool current_color = (record->is_bot_first ^ record->is_on_bot);
			int isselfwin = (winner ^ current_color) ? -1 : 1;
			run_board->update_struct(isselfwin, record);
			delete record;
		}
		run_board->struct_write();
		delete run_board;
	}
	return 0;
}

int main() {
	srand(time(NULL));
	int mode = -1;
	while (mode == -1) {
		cout << "请选择模式(0=训练, 1=人机):";
		cin >> mode;
		if (mode < 0 || mode > 1) {
			cout << "Illegal input!" << endl;
			if (cin.fail()) {
				cin.sync();
				cin.clear();
				cin.ignore();
			}
			mode = -1;
		}
	}
	if (mode == 1) {
		pvc();
	}
	else {
		struct_train();
	}
	return 0;
}