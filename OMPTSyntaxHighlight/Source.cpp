#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <sstream>
#include <regex>
#include "clipboardxx.hpp"

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

CLIOptions ParseCommandLine(int argc, char* argv[]);
std::vector<std::string> Split(const std::string& s, char delimiter);
std::string GetSGRCode(int color);
int GetEffectCmdColor(char c, std::string f);
int GetVolumeCmdColor(char c);
int GetInstrumentColor(char c);
int GetNoteColor(char c);
bool isWhitespace(char c);
bool StartsWith(const char* pre, const char* str);

int main(int argc, char* argv[])
{
	int ColorArgIndex = 0;
	
	// Find the last provided argument and set its index as the color argument index
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-') ColorArgIndex = i;
	}

	// Parse the cli options
	CLIOptions Options = ParseCommandLine(argc, argv);
	
	// Show help (and then exit) if the help option is provided
	if (Options.HELP)
	{
		std::cout << HELP_MESSAGE;
		return 0;
	}

	// Use the first non-option command-line argument as the list of colors
	int Colors[8];
	try
	{
		std::vector<std::string> ColorArgs = Split(argv[ColorArgIndex], ',');
		for (int i = 0; i < ColorArgs.size(); i++)
		{
			Colors[i] = std::stoi(ColorArgs[i]);
			if (Colors[i] < 0 || Colors[i] > 15) throw std::exception("Color value out of range");
		}
	}
	catch (std::exception e)
	{
		if (!Options.USE_STDOUT) std::cout << e.what() << std::endl;
		for (int i = 0; i < 8; i++)
		{
			Colors[i] = DEFAULT_COLORS[i];
		}
	}

	// Read clipboard/STDIN
	std::string Input;
	std::vector<std::string> Lines;
	if (Options.USE_STDIN)
	{
		std::string Line;
		while (std::getline(std::cin, Line))
		{
			Lines.push_back(Line);
			if (Line == "") break;
		}

		for (int i = 0; i < Lines.size(); i++)
		{
			Input += Lines[i];
			if (i != Lines.size() - 1) Input += '\n';
		}
	}
	else
	{
		clipboardxx::clipboard clipboard;
		clipboard >> Input;
	}
	
	// Try to get the module format and check if the data is valid OpenMPT pattern data
	std::string Format;
	Format = Input.substr(HEADER.length(), 3);
	if (!(std::find(FORMATS_M.begin(), FORMATS_M.end(), Format) != FORMATS_M.end() || std::find(FORMATS_S.begin(), FORMATS_S.end(), Format) != FORMATS_S.end()))
	{
		std::cout << "Input does not contain OpenMPT pattern data.";
		return 2;
	}
	
	// Remove colors if the input is already syntax-highlighted
	Input = std::regex_replace(Input, std::regex("\u001B\\[\\d+(;\\d+)*m"), "");

	// Add colors if reverse mode is not enabled
	std::string Output;
	if (!Options.REVERSE_MODE)
	{
		std::string resultbuilder;
		int RelPos = -1;
		int Color = -1;
		int PreviousColor = -1;
		
		for (int i = 0; i < Input.length(); i++)
		{
			char c = Input[i];
			if (c == '|') RelPos = 0;

			if (RelPos == 0) Color = Colors[7];
			if (RelPos == 1) Color = Colors[GetNoteColor(c)];
			if (RelPos == 4) Color = Colors[GetInstrumentColor(c)];
			if (RelPos == 6) Color = Colors[GetVolumeCmdColor(c)];

			if (RelPos >= 9)
			{
				if (RelPos % 3 == 0) Color = Colors[GetEffectCmdColor(c, Format)];
				if (RelPos % 3 != 0 && c == '.' && Input[i - (RelPos % 3)] != '.') c = '0';
			}

			if (!isWhitespace(c))
			{
				if (Color != PreviousColor) resultbuilder += GetSGRCode(Color);
				PreviousColor = Color;
			}

			resultbuilder += c;
			if (RelPos >= 0) RelPos++;
		}
		
		Output = resultbuilder;
	}
	else Output = Input;

	// Wrap in code block for Discord if specified
	if (Options.AUTO_MARKDOWN && !Options.REVERSE_MODE) Output = "```ansi\n" +  Output + "```";

	// Write to clipboard/STDOUT
	if (Options.USE_STDOUT) std::cout << Output;
	else
	{
		clipboardxx::clipboard clipboard;
		clipboard << Output;
	}
}

bool StartsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

CLIOptions ParseCommandLine(int argc, char* argv[])
{
	CLIOptions options;
	for (int i = 1; i < argc; i++)
	{
		if (StartsWith("--", argv[i]))
		{
			if (strcmp(argv[i], "--help") == 0)					options.HELP = true;
			else if (strcmp(argv[i], "--stdin") == 0)			options.USE_STDIN = true;
			else if (strcmp(argv[i], "--stdout") == 0)			options.USE_STDOUT = true;
			else if (strcmp(argv[i], "--markdown") == 0)		options.AUTO_MARKDOWN = true;
			else if (strcmp(argv[i], "--reverse") == 0)			options.REVERSE_MODE = true;

		}
		else if (StartsWith("-", argv[i]))
		{
			for (int j = 1; j < strlen(argv[i]); j++)
			{
				if (argv[i][j] == 'h')							options.HELP = true;
				else if (argv[i][j] == 'i')						options.USE_STDIN = true;
				else if (argv[i][j] == 'o')						options.USE_STDOUT = true;
				else if (argv[i][j] == 'm')						options.AUTO_MARKDOWN = true;
				else if (argv[i][j] == 'r')						options.REVERSE_MODE = true;

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

bool isWhitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

std::string GetSGRCode(int color)
{
	return "\u001B[" + std::to_string(color + ((color < 8) ? 30 : 82)) + "m";
}

int GetNoteColor(char c)
{
	return (c >= 'A' && c <= 'G') ? 1 : 0;
}

int GetInstrumentColor(char c) 
{
	return c >= '0' ? 2 : 0;
}

int GetVolumeCmdColor(char c)
{
	int color = 0;
	
	if (c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'v') color = 3;
	if (c == 'l' || c == 'p' || c == 'r')						  color = 4;
	if (c == 'e' || c == 'f' || c == 'g' || c == 'h' || c == 'u') color = 5;

	return color;
}

int GetEffectCmdColor(char c, std::string f) 
{
	int color = 0;
	if (std::find(FORMATS_S.begin(), FORMATS_S.end(), f) != FORMATS_S.end())
	{
		if (c == 'D' || c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'R')					color = 3;
		else if (c == 'P' || c == 'X' || c == 'Y')													color = 4;
		else if (c == 'E' || c == 'F' || c == 'G' || c == 'H' || c == 'U' || c == '+' || c == '*')	color = 5;
		else if (c == 'A' || c == 'B' || c == 'C' || c == 'T' || c == 'V' || c == 'W')				color = 6;
	}
	else if (std::find(FORMATS_M.begin(), FORMATS_M.end(), f) != FORMATS_M.end())
	{
		if (c == '5' || c == '6' || c == '7' || c == 'A' || c == 'C')		color = 3;
		else if (c == '8' || c == 'P' || c == 'Y')							color = 4;
		else if (c == '1' || c == '2' || c == '3' || c == '4' || c == 'X')	color = 5;
		else if (c == 'B' || c == 'D' || c == 'F' || c == 'G' || c == 'H')	color = 6;
	}
	
	return color;
}