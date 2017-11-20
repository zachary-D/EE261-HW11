#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
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
	quitting,			//The player has requested the program quit
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

	coordi operator+(coordi & other)
	{
		return coordi(x + other.x, y + other.y);
	}
};

//Idea: Don't actually store a 2-d grid of ships, store a vector of ships and instead have their beginning and ending points tracked as part of them.  If a shot falls within
		//their area, count that as a shot.

enum cellContents_type {	//What a given cell on the game board contains (either ocean or ship type)
	null,					//No data
	ocean,					//The ocean			'~' on screen
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
	file_lineTooLong,	//reading from file - line too long (but the error was recovered from, missing data replaced with empty space)

	buffer_xToSmall,	//Screen buffer - x value for size is too small
	buffer_yToSmall,	//Screen buffer - y value for size is too small
	buffer_write_badX,	//Screen buffer - writing to screen - x is out of bounds
	buffer_write_badY,	//Screen buffer - writing to screen - y is out of bounds

	convert_fail_intStr,		//There was an error converting an integer to a string
};

namespace utilities
{
	cellContents_type toCellType(char data)				//Converts a char to a cellContents_type 
	{
		switch(data)
		{
			case '~':
				return ocean;

			case '#':
				return ship;

			case 'H':
				return destroyed_ship;
		}
		return ocean;		//Returns ocean as a last resort/error recovery method
	}

	char toChar(cellContents_type data)					//Converts cellContents_type to a char
	{
		switch(data)
		{
			case ocean:
				return '~';

			case ship:
				return '~';

			case destroyed_ship:
				return '%';
		}

		return toChar(ocean);	//Returns an ocean tile as a last resort/error recovery method
	}

	vector<cellContents_type> extractCellsFromString(string str)	//Extracts data from 'str' and converts it to a vector of 'cellContents_type' elements.  Ignores spaces.
	{
		vector<cellContents_type> data;
		for(auto iter = str.begin(); iter != str.end(); iter++)
		{
			if(*iter != ' ') data.push_back(toCellType(*iter));
		}
		return data;
	}

	vector<string> separateStringsBySpaces(string str)
	{
		vector<string> ret;

		string data = "";

		for(auto iter = str.begin(); iter != str.end(); iter++)
		{
			if(*iter == ' ')
			{
				ret.push_back(data);
				data == "";
			}
			else data.push_back(*iter);
		}
		if(data != "") ret.push_back(data);

		return ret;
	}

	string errorStateToString(errorstates err)
	{
		switch(err)
		{
			case noerror:
				return "No error was encountered.";

			case file_notFound:
				return "File: File not found";

			case file_eof:
				return "File: File ended unexpectedly.";

			case file_eof_fatal:
				return "File: File ended unexpectedly (was not able to recover).";

			case file_lineTooShort:
				return "File: The line ended unexpectedly.";

			case file_lineTooLong:
				return "File: The line was longer than expected.";

			case buffer_xToSmall:
				return "Screen buffer: Attempted to create a buffer with width < 1";

			case buffer_yToSmall:
				return "Screen buffer: Attempted to create a buffer with height < 1";

			case buffer_write_badX:
				return "Screen buffer: Attempted to write to a position that does not exist (x < 0)";
			
			case buffer_write_badY:
				return "Screen buffer: Attempted to write to a position that does not exist (x > buffer size)";

			case convert_fail_intStr:
				return "Data conversion: Integer -> String: Conversion failed.";
		}

		return "Unknown error.";
	}

	string toString(int value)
	{
		string ret;

		std::stringstream convert;
		convert << value;
		convert >> ret;

		if(convert.fail()) throw convert_fail_intStr;

		return ret;
	}
};

void clearConsole()
{
	system("cls");
}

class screenBuffer_type		//The buffer characters are written to before they are written to the screen.  Used to only change portions of the screen at once
{
	coordi size;
	vector<vector<char>> buffer;

public:
	void setSize(coordi _size)
	{
		if(_size.x < 1) throw buffer_xToSmall;
		if(_size.y < 1) throw buffer_yToSmall;
		size = _size;
	
		//While the buffer is larger than it should be
		while(size.x < buffer.size())
		{
			buffer.pop_back();
		}

		//While the buffer is smaller than it should be
		while(size.x > buffer.size())
		{
			buffer.push_back(vector<char>());
		}

		for(int i = 0; i < buffer.size(); i++)
		{
			//While the number of elements is larger than it should be
			while(size.y < buffer[i].size())
			{
				buffer[i].pop_back();
			}

			//While the number of elements is smaller than it should be
			while(size.y > buffer[i].size())
			{
				buffer[i].push_back(' ');
			}
		}
	}

	void pushToConsole()
	{
		clearConsole();
		for(int y = 0; y < size.y; y++)
		{
			for(int x = 0; x < size.x; x++)
			{
				std::cout << buffer[x][y];
			}
			std::cout << std::endl;
		}
	}
	
	void write(coordi pos, char value, bool noFail = false)	//Writes a character ('value') to the buffer at 'pos', if noFail == true, the function will not throw any exceptions
	{
		if(!(0 <= pos.x && pos.x < size.x))
		{
			if(noFail) return;
			else throw buffer_write_badX;
		}
		if(!(0 <= pos.y && pos.y < size.y))
		{
			if(noFail) return;
			else throw buffer_write_badY;
		}

		buffer[pos.x][pos.y] = value;
	}

	void write(coordi pos, string value, bool noFail = false)	//Writes a string ('value') to the buffer, starting at 'pos'
		//If noFail == true, the function will not throw any exceptions, and instead will write as much as it can, giving up if it cannot do something
	{
		for(int i = 0; i < value.size(); i++)
		{
			write(pos + coordi(i, 0), value[i], noFail);
		}
	}

	void clearRow(int y, char clearWith = ' ')		//Replaces an entire row with 'clearWidth', which is a space by default
	{
		for(int x = 0; x < size.x; x++)
		{
			buffer[x][y] = clearWith;
		}
	}
};

screenBuffer_type screen;

class gameBoard_type		//A game board.  Set up as a class so 2-person play is possible, and also to add data validation functions (i.e. don't let things read/write to [-1, 6], etc)
{							//The board also contains some other gameplay data, i.e. number of shots remaining
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
				iter->push_back(ocean);
			}
		}
	}

private:
	vector<vector<cellContents_type>> board;	//The game board, indexed by x and y coordinates
	coordi size;								//The dimensions of the game board

	int shots;		//The number of shots the player has taken
	int shotsMax;	//The maximum number of shots the player can take

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

	bool createShip(coordi startingPoint, direction_type direction = north, int length = 1)
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

				if(row.size() > size.x)	//If the data from the file is larger than expected
				{
					fileErrors.push_back(file_lineTooLong);
				}
				else if(row.size() < size.x)	//If the data from the file is shorter than expected
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
					board[x][y] = ocean;
				}
			}
		}

		printErrors();
	}


	void loadFromFile(string filename)		//Loads the game board from a file, 'filename' 
	{
		ifstream file;
		file.open(filename);
		loadFromFile(file);	
	}

	void print()
	{
		coordi displacement = coordi(3, 2);		//The coordinates of the upper left portion of the board on the buffer

		for(int y = 0; y < size.y; y++)
		{
			for(int x = 0; x < size.x; x++)
			{
				screen.write(coordi(x * 2, y) + displacement, utilities::toChar(getContents(coordi(x, y))));
			}
		}
	}

	void printErrors()
	{
		if(fileErrors.size() > 0)
		{
			cout << "The following errors were encountered when loading the file, but were recovered from:" << endl;
			for(auto iter = fileErrors.begin(); iter != fileErrors.end(); iter++)
			{
				cout << utilities::errorStateToString(*iter) << endl;
			}

			cout << endl << "Press enter to continue" << endl;
			string inp;
			getline(cin, inp);
		}
	}
};

gameBoard_type gameBoard(coordi(25, 25));

void promptUserToResizeWindow()		//Prompts the user to resize their window so that it is the correct height for the game
{
	cout << "You should be able to see this message along with the bottom one." << endl;

	for(int i = 1; i < 28; i++)
	{
		cout << endl;
	}
	cout << "Please resize your window so that you can see this message and the" << endl << "message at the top at the same time." << endl;
	cout << "Please press enter when you're done.";
	{	//Waits until the user presses enter to continue.  I would use cin.ignore(numeric_limits<streamsize>::max(), '\n'), but numeric_limits is missing for some reason
		string inp;
		getline(cin, inp);
	}
	clearConsole();
}

void setup()		//General startup actions
{
	promptUserToResizeWindow();
	gameState = running;

	//Load level data
	{
		cout << "Attempting to load level data..." << endl;
		try
		{
			gameBoard.loadFromFile("levelData.dat");
		}
		catch(errorstates error)
		{
			if(error == file_notFound) cout << "An error was encoutered: The file could not be found." << endl;
			else cout << "An unspecified error was encountered." << endl;
		}

	}

	screen.setSize(coordi(77, 30));

	//Draw border for game board
	{
		const int xMin = 2;
		const int xMax = 52;
		const int yMin = 1;
		const int yMax = 27;

		//Top and bottom border
		for(int x = xMin; x < xMax; x++)
		{
			screen.write(coordi(x, yMin), char(205));
			screen.write(coordi(x, yMax), char(205));
		}

		//The left and right border
		for(int y = yMin; y < yMax; y++)
		{
			screen.write(coordi(xMin, y), char(186));
			screen.write(coordi(xMax, y), char(186));
		}

		//The corners
		screen.write(coordi(xMin, yMin), char(201));	//Top left
		screen.write(coordi(xMax, yMin), char(187));	//Top right
		screen.write(coordi(xMin, yMax), char(200));	//Bottom left
		screen.write(coordi(xMax, yMax), char(188));	//Bottom right

		//game board title 
		screen.write(coordi(xMin + 19, yMin), char(185));	//The left and right borders for the title
		screen.write(coordi(xMin + 30, yMin), char(204));
		screen.write(coordi(xMin + 20, yMin), "Game Board");
	}

	//Write the coordinate indexes along the edge of the board
	{
		//Letters along the top
		for(int x = 0; x < 25; x++)
		{
			screen.write(coordi(x * 2 + 3, 0), char(65 + x));
		}

		//Numbers down the side 
		for(int y = 0; y < 25; y++)
		{
			screen.write(coordi(0, y+2), utilities::toString(y));
		}
	}

	//Draw the instructions to the right side of the screen
	{
		const int menuX = 54;
		const int menuY = 2;

		screen.write(coordi(menuX, menuY), "Key:");
		screen.write(coordi(menuX, menuY + 1), "~ = ocean tile");
		screen.write(coordi(menuX, menuY + 2), "H = hit");
		screen.write(coordi(menuX, menuY + 3), "M = miss");

		screen.write(coordi(menuX, menuY + 5), "Commands:");
		screen.write(coordi(menuX, menuY + 6), "help");
		screen.write(coordi(menuX, menuY + 7), "   shows help");

		screen.write(coordi(menuX, menuY + 9), "fire <letter><number>");
		screen.write(coordi(menuX, menuY + 10), "   fires at the");
		screen.write(coordi(menuX, menuY + 11), "   specified location");
		screen.write(coordi(menuX, menuY + 12), "   ex: fire A5");

		screen.write(coordi(menuX, menuY + 14), "Shots remaining:");
		//screen.write(coordi(menuX, menuY + 15), "  " + utilities::toString(60)));
	}

	screen.write(coordi(0, 29), "Please enter a command, Admiral.");

}

void gameplayLoop()
{
	//Push the newest data to the screen
	gameBoard.print();
	screen.pushToConsole();

	string input;
	getline(cin, input);	//get input from the user

	vector<string> command = utilities::separateStringsBySpaces(input);

	screen.clearRow(28);		//Clear the row used for feedback
	if(command.size() == 0)
	{
		//The user typed nothing
	}


}


void mainLoop()
{
	while(gameState != quitting)
	{
		switch(gameState)
		{
			case title:

				break;

			case running:
				gameplayLoop();

				break;
		}
	}
}

int main()
{
	setup();


	mainLoop();
	



	cin.get();
}