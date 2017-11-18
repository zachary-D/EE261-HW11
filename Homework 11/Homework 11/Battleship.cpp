#include <iostream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::endl;

using std::vector;

struct coordi	//A coordinate pair of integers
{
	coordi::coordi() {}
	coordi::coordi(int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	int x;
	int y;
};

//Idea: Don't actually store a 2-d grid of ships, store a vector of ships and instead have their beginning and ending points tracked as part of them.  If a shot falls within
		//their area, count that as a shot.

enum cellContents_type {	//What a given cell on the game board contains
	empty,					// (either ocean or ship type)
	ship_frigate,
	ship_tender,
	ship_destroyer,
	ship_cruiser,
	ship_carrier
};

class gameBoard_type		//A game board.  Set up as a class so 2-person play is possible, and also to add data validation functions (i.e. don't let things read/write to [-1, 6], etc)
{
public:
	gameBoard_type::gameBoard_type() {}

private:
	vector<vector<cellContents_type>> board;	//The game board, indexed by x and y coordinates

	bool autoExpandBoard = false;		//If true, 'board' will automatically be expanded as needed to write data to/read data from it.  If false, 'maxSize' controls the board size
	coordi maxSize;						//The maximum dimensions of the game board, inclusivley.  Enabled/disabled by 'autoExpandBoard'

public:
	void emptyBoard()		//Empties the game board.  WILL RESULT IN DATA LOSS
	{
		while(board.size() > 0) board.pop_back();
	}

	//Used to lock or unlock the size of the game board
	void lockBoardSize() { autoExpandBoard = false; }
	void unlockBoardSize() { autoExpandBoard = true; }

	void setBoardSize(coordi size)	//Sets the size of the game board to 'size', and locks it at that size.  //MAY RESULT IN DATA LOSS IF THE SIZE OF 'board' > 'size'
	{
		maxSize = size;
		lockBoardSize();
		
		//Trim or expand the board to size
		{
		//Trim or expand the x-axis to size
			while(board.size() > maxSize.x)
			{
				//Remove columns from the end of the x-axis vector
				board.pop_back();
			}
			while(board.size() < maxSize.x)
			{
				//Add empty columns to the x-axis vector
				board.push_back(vector<cellContents_type>());
			}

			//Iterate over the x-axis
			for(auto iter = board.begin(); iter != board.end(); iter++)
			{
				//Trim or expand the elements in each of the y-axies to the proper size
				while(iter->size() > size.y)
				{
					//Remove elements from the the end of the y vector
					iter->pop_back();
				}
				while(iter->size() < size.y)
				{
					//Add elements to the y vector
					iter->push_back(empty);
				}
			}
		}
	}

	cellContents_type getContents(coordi pos)
	{
		if(pos.x < 0) throw "Cannot index negative x coordinate!";
		if(pos.y < 0) throw "Cannot index negative y coordinate!";
		if(pos.x > board.size()) throw "Cannot index x coordinate greater than x-axis size!";
		if(pos.y > board[pos.x].size()) throw "Cannot index y coordinate greatr than y-axis size!";
	}
};

int main()
{

	gameBoard_type gameboard;


	cin.get();
}