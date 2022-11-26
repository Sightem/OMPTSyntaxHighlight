#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <sstream>
#include <regex>
#include <Windows.h>

struct CLIOptions
{
	bool HELP = false;
	bool USE_STDIN = false;
	bool USE_STDOUT = false;
	bool AUTO_MARKDOWN = false;
	bool REVERSE_MODE = false;
};

const std::string HELP_MESSAGE =
"Usage: [EXEC] [OPTIONS] [COLORS]                                              \n"
"                                                                              \n"
"Options:                                                                      \n"
"-h | --help       Help (display this screen)                                  \n"
"-i | --stdin      Read input from STDIN instead of clipboard                  \n"
"-o | --stdout     Write output to STDOUT instead of clipboard                 \n"
"-d | --markdown   Wrap output in Markdown code block (for Discord)            \n"
"-r | --reverse    Reverse mode (removes syntax highlighting instead of adding)\n"
"--                End of options (next argument is treated as list of colors) \n"
"                                                                              \n"
"Using markdown does nothing if reverse mode is enabled.                       \n"
"                                                                              \n"
"Colors:                                                                       \n"
"X,X,X,X,X,X,X,X  Each value from 0 to 15 (Discord only supports 0 to 7)       \n"
"format: Default,Note,Instrument,Volume,Panning,Pitch,Global,ChannelSeparator  \n"
"if not provided: 7,5,4,2,6,3,1,7                                              \n";

const int DEFAULT_COLORS[] = { 7, 5, 4, 2, 6, 3, 1, 7 };
const std::string HEADER = "ModPlug Tracker ";
std::vector<std::string> FORMATS_M = { "MOD", " XM" };
std::vector<std::string> FORMATS_S = { "S3M", " IT", "MPT" };


bool StartsWith(const char* pre, const char* str);
CLIOptions ParseCommandLine(int argc, char* argv[]);
std::vector<std::string> Split(const std::string& s, char delimiter);

int main(int argc, char* argv[])
{
	int ColorArgIndex = 0;
	while (ColorArgIndex < argc)
	{
		if (!StartsWith("-", argv[ColorArgIndex])) break;
		ColorArgIndex++;
		if (StartsWith("--", argv[ColorArgIndex - 1])) break;
	}

	CLIOptions Options = ParseCommandLine(argc, argv);
	
	if (Options.HELP)
	{
		std::cout << HELP_MESSAGE;
		return 0;
	}

	//diabolical code ahead
	int Colors[8];
	try
	{
		std::vector<std::string> ColorArgs = Split(argv[ColorArgIndex], ',');
		for (int i = 0; i < ColorArgs.size(); i++)
		{
			Colors[i] = std::stoi(ColorArgs[i]);
			if (Colors[i] < 0 || Colors[i] > 15)
			{
				throw std::exception("Color value out of range");
			}
		}
	}
	catch (std::exception e)
	{
		if (!Options.USE_STDOUT) std::cout << "Colors not provided properly. Default colors will be used.";
		for (int i = 0; i < 8; i++)
		{
			Colors[i] = DEFAULT_COLORS[i];
		}
	}

	//example input:
	/*
	ModPlug Tracker  IT
|F-510......|A-510......|C-610......|...........|........D01|........M20
|...........|...........|...........|...........|........D01|A-509......
|===........|===........|===........|...........|........D01|...........
|F-510......|A-510......|C-610......|...........|........D01|........D01
|===..v32...|===..v32...|===..v32...|...........|D-609...F06|........D01
|...........|...........|...........|...........|E-6.....G01|........D01
|F-510...Z7E|A-510...Z7E|C-610...Z7E|...........|...........|........D01
|........Z7C|........Z7C|........Z7C|...........|...........|D-609...F06
	
	
	*/
	//read from stdin or clipboard depending on options
	std::string Input;
	if (Options.USE_STDIN)
	{
		//do this later 
	}
	else
	{
		if (!OpenClipboard(NULL))
		{
			std::cout << "Failed to open clipboard";
			return 1;
		}
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (hData == NULL)
		{
			std::cout << "Failed to get clipboard data";
			return 1;
		}
		char* pszText = static_cast<char*>(GlobalLock(hData));
		if (pszText == NULL)
		{
			std::cout << "Failed to lock clipboard data";
			return 1;
		}
		Input = pszText;
		GlobalUnlock(hData);
		CloseClipboard();
	}

	std::string Format;
	
	Format = Input.substr(HEADER.length(), 3);

	if (!(std::find(FORMATS_M.begin(), FORMATS_M.end(), Format) != FORMATS_M.end() || std::find(FORMATS_S.begin(), FORMATS_S.end(), Format) != FORMATS_S.end()))
	{
		std::cout << "Input does not contain OpenMPT pattern data.";
		return 2;
	}

	//find occurences of \u001B\\[\\d+(;\\d+)*m and replace with ""
	
	Input = std::regex_replace(Input, std::regex("\u001B\\[\\d+(;\\d+)*m"), "");

	std::cout << Input << std::endl;

	

	std::cout << "HELP: " << Options.HELP << std::endl;
	std::cout << "USE_STDIN: " << Options.USE_STDIN << std::endl;
	std::cout << "USE_STDOUT: " << Options.USE_STDOUT << std::endl;
	std::cout << "AUTO_MARKDOWN: " << Options.AUTO_MARKDOWN << std::endl;
	std::cout << "REVERSE_MODE: " << Options.REVERSE_MODE << std::endl;

}

bool StartsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre),
		lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

CLIOptions ParseCommandLine(int argc, char* argv[])
{
	CLIOptions options;
	for (int i = 1; i < argc; i++)
	{
		if (StartsWith("--", argv[i]))
		{
			if (strcmp(argv[i], "--help") == 0)
			{
				options.HELP = true;
			}
			else if (strcmp(argv[i], "--stdin") == 0)
			{
				options.USE_STDIN = true;
			}
			else if (strcmp(argv[i], "--stdout") == 0)
			{
				options.USE_STDOUT = true;
			}
			else if (strcmp(argv[i], "--auto-markdown") == 0)
			{
				options.AUTO_MARKDOWN = true;
			}
			else if (strcmp(argv[i], "--reverse") == 0)
			{
				options.REVERSE_MODE = true;
			}
		}
		else if (StartsWith("-", argv[i]))
		{
			for (int j = 1; j < strlen(argv[i]); j++)
			{
				if (argv[i][j] == 'h')
				{
					options.HELP = true;
				}
				else if (argv[i][j] == 'i')
				{
					options.USE_STDIN = true;
				}
				else if (argv[i][j] == 'o')
				{
					options.USE_STDOUT = true;
				}
				else if (argv[i][j] == 'm')
				{
					options.AUTO_MARKDOWN = true;
				}
				else if (argv[i][j] == 'r')
				{
					options.REVERSE_MODE = true;
				}
			}
		}
	}
	return options;
}

std::vector<std::string> Split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

std::string GetSGRCode(int color)
{
	int n = color + ((color < 8) ? 30 : 82);
	return "\u001B[" + std::to_string(n) + "m";
}

int GetNoteColor(char c)
{
	return (c >= 'A' && c <= 'G') ? 1 : 0;
}

int GetInstrumentColor(char c) {
	return c >= '0' ? 2 : 0;
}

int GetEffectCmdColor(char c, std::string f) 
{
	int color = 0;
	if (std::find(FORMATS_S.begin(), FORMATS_S.end(), f) != FORMATS_S.end())
	{
		//do this later
	}
	
	return color;
}