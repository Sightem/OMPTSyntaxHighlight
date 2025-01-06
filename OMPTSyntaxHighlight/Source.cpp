#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <sstream>
#include <regex>
#include <array>
#include <cstring>
#include "clipboardxx.hpp"

struct CLIOptions
{
	bool HELP = false;
	bool USE_STDIN = false;
	bool USE_STDOUT = false;
	bool AUTO_MARKDOWN = false;
	bool REVERSE_MODE = false;
};

constexpr std::string_view HELP_MESSAGE =
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

constexpr std::array DEFAULT_COLORS = { 7, 5, 4, 2, 6, 3, 1, 7 };
constexpr std::string_view HEADER = "ModPlug Tracker ";
constexpr std::array<std::string_view, 2> FORMATS_M = { "MOD", " XM" };
constexpr std::array<std::string_view, 3>  FORMATS_S = { "S3M", " IT", "MPT" };

CLIOptions ParseCommandLine(int argc, char* argv[]);
std::vector<std::string> Split(std::string_view s, char delimiter);
std::string GetSGRCode(int color);
int GetEffectCmdColor(char c, std::string_view f);
int GetVolumeCmdColor(char c);
int GetInstrumentColor(char c);
int GetNoteColor(char c);
bool isWhitespace(char c);
bool StartsWith(std::string_view pre, std::string_view str);

int main(int argc, char* argv[])
{
	int ColorArgIndex = 0;
	
	// Find the last provided argument and set its index as the color argument index
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-') ColorArgIndex = i;
	}

	// Parse the cli options
	auto [HELP, USE_STDIN, USE_STDOUT, AUTO_MARKDOWN, REVERSE_MODE] = ParseCommandLine(argc, argv);

	// Show help (and then exit) if the help option is provided
	if (HELP)
	{
		std::cout << HELP_MESSAGE;
		return 0;
	}

	// Use the first non-option command-line argument as the list of colors
	std::array<int, 8> Colors{};
	try
	{
		const std::vector<std::string> ColorArgs = Split(argv[ColorArgIndex], ',');
		for (int i = 0; i < ColorArgs.size(); i++)
		{
			Colors[i] = std::stoi(ColorArgs[i]);
			if (Colors[i] < 0 || Colors[i] > 15)
				throw std::logic_error("Color value out of range");
		}
	}
	catch (const std::exception& e)
	{
		if (!USE_STDOUT)
			std::cout << e.what() << std::endl;
		for (int i = 0; i < 8; i++)
		{
			Colors[i] = DEFAULT_COLORS[i];
		}
	}

	// Read clipboard/STDIN
	std::string Input;
	if (USE_STDIN)
	{
		std::vector<std::string> Lines;
		std::string Line;
		while (std::getline(std::cin, Line))
		{
			Lines.push_back(Line);
			if (Line.empty())
				break;
		}

		for (int i = 0; i < Lines.size(); i++)
		{
			Input += Lines[i];
			if (i != Lines.size() - 1)
				Input += '\n';
		}
	}
	else
	{
		clipboardxx::clipboard clipboard;
		clipboard >> Input;
	}

	// Try to get the module format and check if the data is valid OpenMPT pattern data
	const std::string Format = Input.substr(HEADER.length(), 3);
	if (!(std::ranges::find(FORMATS_M, Format) != FORMATS_M.end() || std::ranges::find(FORMATS_S, Format) != FORMATS_S.end()))
	{
		std::cout << "Input does not contain OpenMPT pattern data.";
		return 2;
	}

	// Remove colors if the input is already syntax-highlighted
	Input = std::regex_replace(Input, std::regex("\u001B\\[\\d+(;\\d+)*m"), "");

	// Add colors if reverse mode is not enabled
	std::string Output;
	if (!REVERSE_MODE)
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
	else
		Output = Input;

	// Wrap in code block for Discord if specified
	if (AUTO_MARKDOWN && !REVERSE_MODE)
		Output = "```ansi\n" +  Output + "```";

	// Write to clipboard/STDOUT
	if (USE_STDOUT)
		std::cout << Output;
	else
	{
		clipboardxx::clipboard clipboard;
		clipboard << Output;
	}
}

inline bool StartsWith(const std::string_view pre, const std::string_view str)
{
	return str.substr(0, pre.length()) == pre;
}

CLIOptions ParseCommandLine(const int argc, char* argv[])
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

std::vector<std::string> Split(const std::string_view s, const char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s.data());
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

bool isWhitespace(const char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string GetSGRCode(const int color)
{
	return "\u001B[" + std::to_string(color + ((color < 8) ? 30 : 82)) + "m";
}

int GetNoteColor(const char c)
{
	return (c >= 'A' && c <= 'G') ? 1 : 0;
}

int GetInstrumentColor(const char c)
{
	return c >= '0' ? 2 : 0;
}

int GetVolumeCmdColor(const char c)
{
	int color = 0;
	
	switch (c)
	{
		case 'a': case 'b': case 'c': case 'd': case 'v': color = 3; break;
		case 'l': case 'p': case 'r': color = 4; break;
		case 'e': case 'f': case 'g': case 'h': case 'u': color = 5; break;
	}

	return color;
}

int GetEffectCmdColor(const char c, const std::string_view f)
{
	int color = 0;
	if (std::ranges::find(FORMATS_S, f) != FORMATS_S.end())
	{
		switch (c)
		{
			case 'D': case 'K': case 'L': case 'M': case 'N': case 'R': color = 3; break;
			case 'P': case 'X': case 'Y': color = 4; break;
			case 'E': case 'F': case 'G': case 'H': case 'U': case '+': case '*': color = 5; break;
			case 'A': case 'B': case 'C': case 'T': case 'V': case 'W': color = 6; break;
		}
	}
	else if (std::ranges::find(FORMATS_M, f) != FORMATS_M.end())
	{
		switch (c)
		{
			case '5': case '6': case '7': case 'A': case 'C': color = 3; break;
			case '8': case 'P': case 'Y': color = 4; break;
			case '1': case '2': case '3': case '4': case 'X': color = 5; break;
			case 'B': case 'D': case 'F': case 'G': case 'H': color = 6; break;
		}
	}
	
	return color;
}