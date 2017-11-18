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

enum direction_type {		//A direction
	north,
	south,
	east,
	west
};

class gameBoard_type		//A game board.  Set up as a class so 2-person play is possible, and also to add data validation functions (i.e. don't let things read/write to [-1, 6], etc)
{
public:
	gameBoard_type::gameBoard_type(coordi boardSize) {
		size = boardSize;

		//Trim or expand the x-axis to size
		while(board.size() > size.x)
		{
			//Remove columns from the end of the x-axis vector
			board.pop_back();
		}
		while(board.size() < size.x)
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

private:
	vector<vector<cellContents_type>> board;	//The game board, indexed by x and y coordinates
	coordi size;								//The dimensions of the game board

public:
	void emptyBoard()		//Empties the game board.  WILL RESULT IN DATA LOSS
	{
		//Empty the data from the board
		while(board.size() > 0) board.pop_back();

		//Repopulate the board with empty spaces
		{
			vector<cellContents_type> column;	//An empty column

			//Expand the empty column 'template' to the proper size
			for(int i = 0; i < size.y; i++)
			{
				column.push_back(empty);
			}

			//Insert the empty column into the x-vector to build the chart to the proper width
			for(int i = 0; i < size.x; i++)
			{
				board.push_back(column);
			}
		}

	}

	coordi getBoardSize() { return size; }

	bool isValidPosition(coordi pos)
	{
		return (0 < pos.x && pos.x < size.x) && (0 < pos.y && pos.y < size.y);
	}

	cellContents_type getContents(coordi pos)
	{
		if(isValidPosition(pos)) return board[pos.x][pos.y];
	}

	bool createShip(coordi startingPoint, direction_type direction, int length)
	{

	}
};

int main()
{

	gameBoard_type gameboard(coordi(25, 25));


	cin.get();
}