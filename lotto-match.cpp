#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm> // For std::sort
#include <filesystem>
#include <cctype>

// ANSI escape codes for coloring
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Function to split a string by a delimiter
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Structure to hold information about a matched line
struct MatchedLineInfo {
    std::string outputString; // The formatted string to print
    int matchCount;           // Number of positional matches found in this line
    std::string filename;     // To keep track of which file the line came from
    int originalLineNumber;   // To keep track of the original line number
    std::vector<bool> matchedChosenPositions; // Size 6, true if chosenNumbers[i] matched at position i

    // Custom comparison operator for sorting:
    // 1. Primary: Descending by matchCount
    // 2. Secondary: If matchCount is equal, prioritize based on which chosenNumbers were matched
    //               (leftmost chosen numbers (4, then 6, then 8...) have higher priority)
    // 3. Tertiary: Ascending by filename (if all above are equal)
    // 4. Quaternary: Ascending by originalLineNumber (if all above are equal)
    bool operator<(const MatchedLineInfo& other) const {
        if (matchCount != other.matchCount) {
            return matchCount > other.matchCount; // Primary: Descending by matchCount
        }

        // Secondary: If match counts are equal, compare based on matchedChosenPositions
        for (size_t i = 0; i < matchedChosenPositions.size(); ++i) {
            if (matchedChosenPositions[i] != other.matchedChosenPositions[i]) {
                return matchedChosenPositions[i] > other.matchedChosenPositions[i];
            }
        }

        // Tertiary: If all match flags are also identical, sort by filename alphabetically
        if (filename != other.filename) {
            return filename < other.filename;
        }

        // Quaternary: If filenames are also equal, sort by original line number
        return originalLineNumber < other.originalLineNumber;
    }
};

// Function to process a single CSV file
// Returns a vector of MatchedLineInfo instead of printing directly
std::vector<MatchedLineInfo> processCsvFile(const std::string& filename, const std::vector<int>& chosenNumbers) {
    std::vector<MatchedLineInfo> matchedLinesForFile;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return matchedLinesForFile;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        lineNumber++;

        if (line.empty() || std::isalpha(static_cast<unsigned char>(line[0]))) {
            continue;
        }

        std::vector<std::string> fields = split(line, ';');

        if (fields.empty()) {
            continue;
        }

        std::string dateString;
        if (!fields[0].empty()) {
            dateString = fields[0];
        } else {
            dateString = "(Not found)";
        }

        int currentPositionalMatchCount = 0;
        std::ostringstream numbersOutput;
        std::vector<bool> lineMatchedChosenPositions(chosenNumbers.size(), false);

        for (int i = 1; i <= 6 && i < fields.size(); ++i) {
            try {
                int currentNumber = std::stoi(fields[i]);
                
                if ((i - 1) < chosenNumbers.size() && currentNumber == chosenNumbers[i - 1]) {
                    numbersOutput << ANSI_COLOR_GREEN << currentNumber << ANSI_COLOR_RESET;
                    currentPositionalMatchCount++;
                    lineMatchedChosenPositions[i - 1] = true;
                } else {
                    numbersOutput << currentNumber; // Print normally if no match
                }

                if (i < 6 && i < fields.size() - 1) {
                    numbersOutput << " ";
                }
            } catch (const std::invalid_argument& e) {
                // Silently ignore malformed numbers
            } catch (const std::out_of_range& e) {
                // Silently ignore malformed numbers
            }
        }

        if (currentPositionalMatchCount > 0) {
            MatchedLineInfo info;
            info.outputString = ANSI_COLOR_YELLOW + dateString + ANSI_COLOR_RESET + " " + numbersOutput.str();
            info.matchCount = currentPositionalMatchCount;
            info.filename = filename;
            info.originalLineNumber = lineNumber;
            info.matchedChosenPositions = lineMatchedChosenPositions;
            matchedLinesForFile.push_back(info);
        }
    }

    file.close();
    return matchedLinesForFile;
}

int main() {
    const std::vector<int> chosenNumbers = {4, 6, 8, 18, 21, 26};

    std::cout << "\nSearching... " << chosenNumbers[0] << " " << chosenNumbers[1] << " " << chosenNumbers[2] << " " << 
        chosenNumbers[3] << " " << chosenNumbers[4] << " " << chosenNumbers[5] << std::endl << std::endl;

    std::vector<std::string> csvFiles;

    if (std::filesystem::exists(".")) {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ".csv") {
                csvFiles.push_back(entry.path().filename().string());
            }
        }
    } else {
        std::cerr << "Error: Current directory not found." << std::endl;
        return 1;
    }

    if (csvFiles.empty()) {
        std::cout << "No .csv files found in the current directory." << std::endl;
    } else {
        std::sort(csvFiles.begin(), csvFiles.end());

        std::vector<MatchedLineInfo> allMatchedLines;

        for (const std::string& filename : csvFiles) {
            std::vector<MatchedLineInfo> linesFromFile = processCsvFile(filename, chosenNumbers);
            allMatchedLines.insert(allMatchedLines.end(), linesFromFile.begin(), linesFromFile.end());
        }

        if (allMatchedLines.empty()) {
            std::cout << "No lines with positional matches found across all .csv files." << std::endl;
        } else {
            std::sort(allMatchedLines.begin(), allMatchedLines.end());

            for (const auto& info : allMatchedLines) {
                std::cout << info.outputString << std::endl;
            }
        }

        std::cout << std::endl;
    }

    return 0;
}
