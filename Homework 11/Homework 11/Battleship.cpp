#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::endl;

using std::ifstream;
using std::getline;

using std::string;

using std::vector;

enum gameState_type		//The current state the game is in
{
	title,				//The game is on the title screen
	running,			//The game is running normally
	error,				//The game has encountered an error that cannot be recovered from
	win,				//The player has won
	lose,				//The player has lost
} gameState;

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

enum cellContents_type {	//What a given cell on the game board contains (either ocean or ship type)
	null,					//No data
	empty,					//aka the ocean			'~' on screen
	ship,					//A ship (hidden from the player)
	destroyed_ship,			//The wreckage of a destroyed ship		'%' on screen
	invalid_cell			//An error type, used when data for a cell that does not exits
};

enum direction_type {		//A direction
	north,
	south,
	east,
	west
};

enum errorstates {			//Various errors that can be thrown 
	noerror,			//No error
	file_notFound,		//reading from file - file not found
	file_eof,			//reading from file - file ended too soon (but the error was recovered from, missing data replaced with empty space)
	file_eof_fatal,		//reading from file - file ended too soon (couldn't recover from)
	file_lineTooShort,	//reading from file - line too short (but the error was recovered from, missing data replaced with empty space)
	file_lineTooLong	//reading from file - line too long (but the error was recovered from, missing data replaced with empty space)


};

namespace utilities
{
	cellContents_type toCellType(char data)
	{
		if(data == '~') return empty;
		else if(data == '#') return ship;
		else if(data == 'H') return destroyed_ship;
		else return invalid_cell;
	}

	vector<cellContents_type> extractCellsFromString(string str)	//Extracts data from 'str' and converts it to a vector of 'cellContents_type' elements.  Ignores spaces.
	{
		vector<cellContents_type> data;
		for(auto iter = str.begin(); iter != str.end(); iter++)
		{
			if(*iter != ' ') data.push_back(toCellType(*iter));
		}
	}

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

	//File loader flags
	vector<errorstates> fileErrors;

public:
	void emptyBoard()		//Empties the game board.  WILL RESULT IN DATA LOSS
	{
		//Empty the data from the board
		while(board.size() > 0) board.pop_back();

		//Repopulate the board with null cells spaces
		{
			vector<cellContents_type> column;	//An empty column

			//Expand the empty column 'template' to the proper size
			for(int i = 0; i < size.y; i++)
			{
				column.push_back(null);
			}

			//Insert the empty column into the x-vector to build the chart to the proper width
			for(int i = 0; i < size.x; i++)
			{
				board.push_back(column);
			}
		}

	}

	coordi getBoardSize() { return size; }

	bool isValidPosition(coordi pos)		//Returns true when 'pos' is a valid position within the game board
	{
		return (0 < pos.x && pos.x < size.x) && (0 < pos.y && pos.y < size.y);
	}

	cellContents_type getContents(coordi pos)	//Returns the contents of a cell, after making sure that the cell is valid
	{
		if(isValidPosition(pos)) return board[pos.x][pos.y];
		return invalid_cell;	//It's only possible to reach this part if 'pos' is an invalid position
	}

	bool createShip(coordi startingPoint, direction_type direction, int length)
	{

	}

	void loadFromFile(ifstream & file)		//Loads the game board from 'file'
	{
		if(!file)
		{
			//File does not exist
			throw file_notFound;
		}
		
		//Load each line from the file, and store it in each row
		for(int y = 0; y < size.y; y++)
		{
			string line;
			getline(file, line);

			if(!file.eof())
			{	//If the line from the file was loaded successfully
				vector<cellContents_type> row = utilities::extractCellsFromString(line);

				if(row.size > size.x)	//If the data from the file is larger than expected
				{
					fileErrors.push_back(file_lineTooLong);
				}
				else if(row.size < size.x)	//If the data from the file is shorter than expected
				{
					fileErrors.push_back(file_lineTooShort);
				}

				//Store the data loaded
				for(int x = 0; x < size.x; x++)
				{
					board[x][y] = row[x];
				}
			}
			else
			{
				fileErrors.push_back(file_eof);
				for(int x = 0; x < size.x; x++)
				{
					board[x][y] = empty;
				}
			}
		}
	}


	void loadFromFile(string filename)		//Loads the game board from a file, 'filename' 
	{
		ifstream file;
		file.open(filename);
		loadFromFile(file);	
	}


};

gameBoard_type gameBoard;

void setup()		//General startup actions
{
	gameState = running;
}

int main()
{
	gameBoard_type gameboard(coordi(25, 25));


	cin.get();
}