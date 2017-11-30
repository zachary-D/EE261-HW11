#include <ctime>
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

const bool allowDebugCommands = true;	//Controls whether or not debug commands may be turned on
bool debugCommandsOn = false;			//Controls if debug commands are turned on

//Some debug flags
bool debug_forceRun = false;
bool debug_showShips = false;

int generatorLoopNum = 0;

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

	coordi operator+=(coordi & other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	bool operator==(coordi & other)
	{
		return (x == other.x) && (y == other.y);
	}

	bool operator!=(coordi & other)
	{
		return (x != other.x) || (y != other.y);
	}
};

//Idea: Don't actually store a 2-d grid of ships, store a vector of ships and instead have their beginning and ending points tracked as part of them.  If a shot falls within
		//their area, count that as a shot.

enum cellContents_type {	//What a given cell on the game board contains (either ocean or ship type)
	null,					//No data
	ocean,					//The ocean			'~' on screen
	ship,					//A ship (hidden from the player)
	destroyed_ship,			//The wreckage of a destroyed ship		'%' on screen
	shot_miss,				//A shot that hit nothing (displayed as ocean)
	invalid_cell			//An error type, used when data for a cell that does not exits
};

enum direction_type {		//A direction
	north,
	south,
	east,
	west
};

enum shotResult {		//The possible outcomes of the player firing a shot
	hit,
	miss,
	noAmmo,
	alreadyFired,
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
	convert_fail_strInt,		//There was an error converting a string to an integer
	convert_fail_charInt,		//There was an error converting a char to an integer

	board_badX,			//Game board - The x value is invalid
	board_badY,			//Game board - The y value is invalid

	board_gen_shipExists,	//Game board - board generator - generator attempted to place a ship over another (isn't really an error, but is used to loop if this happens)

	rand_badBounds,		//utilities::rand(), min is greater than or equal to max (no valid values)
};

enum class ASCII		//ASCII characters and their associated integer numbers
{
	ZERO = 48,
	ONE = 49,
	TWO = 50,
	THREE = 51,
	FOUR = 52,
	FIVE = 53,
	SIX = 54,
	SEVEN = 55,
	EIGHT = 56,
	NINE = 57,

	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,

	a = 97,
	b = 98,
	c = 99,
	d = 100,
	e = 101,
	f = 102,
	g = 103,
	h = 104,
	i = 105,
	j = 106,
	k = 107,
	l = 108,
	m = 109,
	n = 110,
	o = 111,
	p = 112,
	q = 113,
	r = 114,
	s = 115,
	t = 116,
	u = 117,
	v = 118,
	w = 119,
	x = 120,
	y = 121,
	z = 122,

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

			case 'M':
				return shot_miss;
		}
		return ocean;		//Returns ocean as a last resort/error recovery method
	}

	char toChar(cellContents_type data, bool showHiddenShips = false)					//Converts cellContents_type to a char
	{
		switch(data)
		{
			case ocean:
				return '~';

			case ship:
				return (showHiddenShips ? '#' : toChar(ocean));	//Display as ocean if they're hidden, otherwise show as '#'

			case destroyed_ship:
				return 'H';

			case shot_miss:
				return 'M';
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

	vector<string> separateStringsBySpaces(string str)		//Separates a string into a vector of strings, using spaces as separators (i.e. "A B C" -> {a,b,c})
	{
		vector<string> ret;

		string data = "";

		for(auto iter = str.begin(); iter != str.end(); iter++)
		{
			if(*iter == ' ')
			{
				ret.push_back(data);
				data = "";
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

	double toNum(char inp, bool noFail = false)
	{
		double ret;

		std::stringstream convert;
		convert << inp;
		convert >> ret;

		if(convert.fail() && noFail == false) throw convert_fail_charInt;

		return ret;
	}

	double toNum(string inp)
	{
		double ret;

		std::stringstream convert;
		convert << inp;
		convert >> ret;

		if(convert.fail()) throw convert_fail_strInt;

		return ret;
	}

	bool isNum(string inp)
	{
		try
		{
			toNum(inp);
		}
		catch(...)			//If any exception is triggered while converting 'inp' to a number, it can be assumed that 'inp' is not a number
		{
			return false;
		}

		return true;
	}

	string toLower(string input)
	{
		for(int i = 0; i < input.size(); i++)
		{
			input[i] = tolower(input[i]);
		}
		return input;
	}

	bool isCharLetter(char inp)	//Is the character 'inp'  a->z or A->Z
	{
		inp = tolower(inp);
		return 'a' <= inp && inp <= 'z';
	}

	bool isCharNum(char inp)	//Is the character the CHARACTER for 0->9, NOT if the char ID is 0->9
	{
		return (int) ASCII::ZERO <= inp && (int) inp <= (int) ASCII::NINE;
	}

	int rand(int lower, int higher)		//Returns a random number between 'lower' and 'higher' (inclusive)
	{
		if(lower >= higher) throw rand_badBounds;
		return std::rand() % (higher - lower + 1) + lower;
	}

	template <typename T> int sgn(T val) {
		//Returns -1 when val < 0, 0 when val = 0, and 1 when val > 0
		//Credit to https://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
		return (T(0) < val) - (val < T(0));
	}
};

namespace util = utilities;

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

	int shotsMax = 60;	//The maximum number of shots the player can take
	int shots = shotsMax;		//The number of shots the player has left

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
		return (0 <= pos.x && pos.x < size.x) && (0 <= pos.y && pos.y < size.y);
	}

	cellContents_type getContents(coordi pos)	//Returns the contents of a cell, after making sure that the cell is valid
	{
		if(!(0 <= pos.x && pos.x < size.x)) throw board_badX;
		if(!(0 <= pos.y && pos.y < size.y)) throw board_badY;
		return board[pos.x][pos.y];
	}

	void setContents(coordi pos, cellContents_type cell)
	{
		if(!(0 <= pos.x && pos.x < size.x)) throw board_badX;
		if(!(0 <= pos.y && pos.y < size.y)) throw board_badY;
		board[pos.x][pos.y] = cell;
	}

	//Places a ship of 'length' at 'startingPoint' in 'direction'
	void createShip(coordi startingPoint, direction_type direction = north, int length = 1)
	{
		coordi size(0, 0);		//The 'size' of the ship, with 'startingPoint' as the origin

		switch(direction)
		{
			case north:
				size.y = -length;
				break;

			case east:
				size.x = length;
				break;

			case south:
				size.y = length;
				break;

			case west:
				size.x = -length;
				break;
		}

		coordi pos = startingPoint;		//The current point

		do		//loop util pos == startingPoint + size, checking for other ships
		{
			if(getContents(pos) == ship)
			{
				generatorLoopNum;
				throw board_gen_shipExists;
			}

			pos += coordi(util::sgn(size.x), util::sgn(size.y));	//Incriment pos in the proper direction
		} while(pos != startingPoint + size);

		pos = startingPoint;

		do		//loop util pos == startingPoint + size, placing ship elements
		{
			setContents(pos, ship);

			pos += coordi(util::sgn(size.x), util::sgn(size.y));	//Incriment pos in the proper direction
		}
		while(pos != startingPoint + size);
	}

	void createShip(int length)		//Randomly places a ship of 'length'
	{
		direction_type dir = direction_type(util::rand(0, 3));

		int xMin = (dir == west ? length : 0);
		int xMax = (dir == east ? size.x - length : size.x - 1);
		int yMin = (dir == north ? length : 0);
		int yMax = (dir == south ? size.y - length : size.y - 1);

		coordi pos = coordi(util::rand(xMin, xMax), util::rand(yMin, yMax));

		createShip(pos, dir, length);
	}

	int getShots() { return shots; }

	void setShots(int value, bool force = false)	//Sets the number of shots remaining to 'value', if value > 0.  if 'force' == true the input validation is ovveridden
	{
		if(value > 0 || force) shots = value;		//If the number of shots remaning is greater than 0, or force (as in force setting) is enabled, set it to the value
	}

	void checkWinLoss()			//Checks the game data for win/loss conditions
	{
		{	//If there are no ships left on the board, the player wins
			bool shipFound = false;

			for(int x = 0; x < size.x; x++)
			{
				for(int y = 0; y < size.y; y++)
				{
					if(getContents(coordi(x, y)) == ship)
					{
						shipFound = true;
						break;
					}
				}
				if(shipFound) break;
			}

			if(shipFound == false) gameState = win;
		}

		if(shots <= 0)	//If the player is out of shots (Loss condition)
		{
			gameState = lose;
			return;
		}


	}

	shotResult fire(coordi at)	//Returns true if the shot hit something, otherwise false
	{
		if(shots <= 0)		//If we have no shots remaining, we cannot fire.
		{
			gameState = lose;
			return noAmmo;
		}

		shots--;

		cellContents_type cell = getContents(at);

		switch(cell)
		{
			case ship:
				setContents(at, destroyed_ship);
				return hit;

			case destroyed_ship:
				return alreadyFired;

			case shot_miss:
				return alreadyFired;

			case ocean:
				setContents(at, shot_miss);
				return miss;

			default:
				setContents(at, shot_miss);
				return miss;
				//If we don't have a case for the cell, assume it's data is bad and set it as a missed shot
		}
	}

	shotResult fire(char _let, int _num)		//Returns true if the shot hit something, otherwise returns false
	{
		return fire(coordi(toupper(_let) - 'A', _num));		//the letter coordinate is determined by toupper(_let) - 'A' because that means when _let == 'A', the result will be 0
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
		file.close();
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

	void print(bool showHiddenShips = false)
	{
		coordi displacement = coordi(3, 2);		//The coordinates of the upper left portion of the board on the buffer

		for(int y = 0; y < size.y; y++)
		{
			for(int x = 0; x < size.x; x++)
			{
				screen.write(coordi(x * 2, y) + displacement, utilities::toChar(getContents(coordi(x, y)), showHiddenShips));
			}
		}

		screen.write(coordi(54 + 2, 2 + 15), "     ", true);
		screen.write(coordi(54 + 2, 2 + 15), utilities::toString(getShots()));
	}

	void generateGameBoard()
	{
		cout << "Please be patient, this may take a second..." << endl;
		emptyBoard();
		
		vector<int> lengths {2, 2, 3, 3, 4};

		for(int iter = 0; iter < lengths.size(); iter++)		//Generate ships with lengths dictated in 'lengths'
		{
			try { createShip(lengths[iter]); }
			catch(...) { iter--; } //If there's any error, just de-incriment the iterator and start over and run the randomizer again.  The issue is unlikely to crop up repeatedly
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
	srand(std::time(NULL));	//Seed the randomizer

	promptUserToResizeWindow();
	gameState = title;

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
			screen.write(coordi(0, y + 2), utilities::toString(y));
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

	}
}

void printPlayerFeedback(string feedback)		//Prints 'feedback' for the player to the specific spot in the screen buffer reserved for it 
{
	screen.clearRow(28);
	screen.write(coordi(0, 28), feedback, true);
}

bool handleDebugCommands(vector<string> command)		//Checks for and executes debug commands in 'command' if they exist.  Returns 'true' if it is a debug command, false if not
{
	if(command.size() == 0) return false;	//If the argument isn't a command, it can't be a debug command

	if(command[0].size() > 0 && command[0][0] == '#' && allowDebugCommands == true)		//Debug command handler (requires that allowDebugCommands be true to enable them)
	{
		command[0] = command[0].substr(1);	//Trims the '#' from the beginning of the first term to make things more readable

		if(debugCommandsOn == false)				//If debug commands are enabled
		{
			if(command[0] == "enable")				//Enables debug commands
			{
				debugCommandsOn = true;
				printPlayerFeedback("Debug commands enabled.  #disable to turn them off.");
				return true;
			}
		}
		else
		{
			if(command[0] == "enable")				//Debug commands are already on, so we just say so. 
			{
				printPlayerFeedback("Debug commands are already enabled.");
				return true;
			}
			else if(command[0] == "disable")		//Disables debug commands
			{
				debugCommandsOn = false;
				printPlayerFeedback("Debug commands are now disabled.");
				return true;
			}
			else if(command[0] == "setshots")		//Sets the number of shots the player has remaining
			{
				if(command.size() >= 2)
				{
					try
					{
						gameBoard.setShots(utilities::toNum(command[1]));
					}
					catch(errorstates err)
					{
						if(err == convert_fail_strInt)
						{
							printPlayerFeedback("\"" + command[1] + "\" could not be converted to an integer.");
						}
						else throw err;
					}

				}
			}
			else if(command[0] == "forcerun")	//Forces the game to run, preventing it from closing
			{
				debug_forceRun = true;
			}
			else if(command[0] == "normalrun")	//Disables forceRun, returns the game to it's normal state
			{
				debug_forceRun = false;
			}
			else if(command[0] == "showships")
			{
				debug_showShips = true;
			}
			else if(command[0] == "hideships")
			{
				debug_showShips = false;
			}
			else if(command[0] == "killall")
			{
				for(int x = 0; x < gameBoard.getBoardSize().x; x++)
				{
					for(int y = 0; y < gameBoard.getBoardSize().y; y++)
					{
						if(gameBoard.getContents(coordi(x, y)) == ship)
						{
							gameBoard.setContents(coordi(x, y), destroyed_ship);
						}
					}
				}
			}
		}
	}
	else return false;

	return true;
}

void lossScreen()
{
	gameBoard.print(true);

	screen.clearRow(28);
	screen.clearRow(29);

	printPlayerFeedback("We've lost, Admiral.");
	screen.write(coordi(0, 29), "Press enter to return to the title screen.");
	screen.pushToConsole();


	getline(cin, string());	//Wait for the user to press enter (getline and store it in a new string that will immediatley be deconstructed)

	gameState = title;


}

void winScreen()
{
	gameBoard.print(true);

	screen.clearRow(28);
	screen.clearRow(29);

	printPlayerFeedback("We've won, Admiral!");
	screen.write(coordi(0, 29), "Press enter to return to the title screen.");
	screen.pushToConsole();


	getline(cin, string());	//Wait for the user to press enter (getline and store it in a new string that will immediatley be deconstructed)

	gameState = title;
}


void mainLoop()
{
	while(gameState != quitting)
	{
		if(gameState == title)
		{
			clearConsole();

			cout << "Please select an option:" << endl;

			cout << "1) Load game board from file" << endl;
			cout << "2) Generate new game board" << endl;
			cout << "3) Quit" << endl;

			string input;

			//Loop until we get valid data from the player
			while(true)
			{
				getline(cin, input);

				if(input.size() == 0 || !(1 <= utilities::toNum(input[0], true) && utilities::toNum(input[0], true) <= 3))
				{
					cout << "I'm sorry, I don't understand \"" << input << "\".  Please try again." << endl;
				}
				else break;
			}

			if(input[0] == '1')
			{
				screen.clearRow(29);
				screen.write(coordi(0, 29), "Please enter a command, Admiral.");
				string filename = "";
				while(true)
				{
					cout << endl << "Please enter a file to open:";

					getline(cin, filename);
					if(filename == "")
					{
						cout << "Please enter a filename." << endl;
					}
					else break;
				}
				//Load the game board from a file

				//Load level data
				{
					cout << "Attempting to load level data..." << endl;
					try
					{
						gameBoard.emptyBoard();
						gameBoard.loadFromFile(filename);// "levelData.dat");
					}
					catch(errorstates error)
					{
						if(error == file_notFound) cout << "An error was encoutered: The file could not be found." << endl;
						else cout << "An unspecified error was encountered." << endl;
						cout << "Generating new game board..." << endl;
						gameBoard.generateGameBoard();
					}
					gameState = running;
				}
			}
			else if(input[0] == '2')
			{
				screen.clearRow(29);
				screen.write(coordi(0, 29), "Please enter a command, Admiral.");
				//Generate a new game board
				gameBoard.generateGameBoard();
				gameState = running;
			}
			else if(input[0] == '3')
			{
				gameState = quitting;
			}

			screen.clearRow(28);		//Clear the row used for feedback on the buffer, in case the game has already been run once

		}
		else
		{
			//Check for win/loss conditions
			gameBoard.checkWinLoss();

			//Push the newest data to the screen
			gameBoard.print(debug_showShips && debugCommandsOn);

			if(gameState == quitting) printPlayerFeedback("gameState == quitting.  Enter #forceRun to prevent the game from closing.");
			screen.pushToConsole();
			
			string input;
			if(gameState == running || debugCommandsOn) getline(cin, input);	//get input from the user

			vector<string> command = utilities::separateStringsBySpaces(utilities::toLower(input));

			screen.clearRow(28);		//Clear the row used for feedback

			if(debug_forceRun && debugCommandsOn) gameState = running;

			switch(gameState)
			{
				case running:
				{
					if(handleDebugCommands(command))
					{
						//Do nothing because the command was already handled
					}
					else if(command.size() == 0)
					{
						//The user typed nothing
					}
					else if(command[0] == "fire")
					{
						if(gameState != running)
						{
							printPlayerFeedback("This command cannot be used at this time.");
						}
						else
						{
							if(command.size() < 2)
							{
								printPlayerFeedback("Please specify a firing coordinate.");
								return;
							}
							else
							{
								string pos = command[1];

								if(pos.size() >= 2) //If the command is at least two characters long
								{
									char first = pos[0];
									string second = pos.substr(1);

									if(!utilities::isCharLetter(first) || !utilities::isNum(second))		//If either part of the coordinate is invalid (if the first character is not a letter, and if the remaining text is not a number
									{

										printPlayerFeedback("Sorry Admiral, firing coordinate \"" + pos + "\" is invalid.  Please try again.");
										return;
									}
									else
									{
										try
										{
											shotResult result = gameBoard.fire(first, utilities::toNum(second));

											gameBoard.checkWinLoss();

											if(result == shotResult::alreadyFired)
											{
												printPlayerFeedback("You already fired on that location.");
											}
											else if(result == shotResult::hit)
											{
												printPlayerFeedback("Confirmed hit Admiral!");
											}
											else if(result == shotResult::miss)
											{
												printPlayerFeedback("We didn't hit anything Admiral.");
											}
										}
										catch(errorstates err)
										{
											if(err == board_badX || err == board_badY)	//If the user fired at an invalid point
											{
												printPlayerFeedback("Sorry Admiral, firing coordinate \"" + pos + "\" is invalid.  Please try again.");
											}
											else throw err;		//If we can't handle it here, just pass it up the 'chain'
										};
									}
								}
								else
								{
									printPlayerFeedback("Sorry Admiral, firing coordinate \"" + pos + "\" is invalid.  Please try again.");
									return;
								}
							}

						}


					}

				}
					break;

				case lose:
					lossScreen();
					break;

				case win:
					winScreen();
					break;
			}
		}
	}
}

int main()
{
	setup();

	mainLoop();
}