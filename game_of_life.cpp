#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>

//using namespace std;

// ----------------------------------------------------------------------------

struct Params 
{
    std::string input_file = "";
    int iterations = 0;
    bool save_all = false;
    bool do_display_board = false; // Additional argument for debug! Not documented!

    void init() 
    {
        input_file = "";
        iterations = 0;
        save_all = false;
        do_display_board = false;
    }
}; 

// ----------------------------------------------------------------------------

class IO_Utils
{
 public:
    //
    static void printError(const std::string &message) 
    {
        std::cout << "\n";    
        std::cout << "<!> ERROR: " << message << "\n";
    }

    //
    static void printSuccess()
    {
        std::cout << "\n";    
        std::cout << "OK." << "\n";
    }

    //
    static void printUsage() 
    {
        std::cout << "\n";    
        std::cout << "Usage: game_of_life --input initial.txt --iterations 2000 [--all]" << "\n";
    }

    //
    static void printStatus(int iteration, const std::string &filename="") 
    {
        std::cout << "Iteration: #" << iteration;
        if (!filename.empty()) 
            std::cout << " ---> Saving board to \"" << filename << "\"...";
        std::cout << "\n"; 
    }
    
    // Parse input arguments...
    static bool parseProgramArguments(int argc, char* argv[], Params &params) 
    {
        //
        if (argc == 2)
        {
            std::string arg = argv[1];
            if (arg == "--h" || arg == "--help")
            {
                // Do not display error message.
                return false;
            }

        }

        params.init();

        for (int i = 1; i < argc; ++i) 
        {
            std::string arg = argv[i];
            if (arg == "--input" && i + 1 < argc) 
            {
                params.input_file = argv[i + 1];
            } 
            else if (arg == "--iterations" && i + 1 < argc) 
            {
                params.iterations = std::stoi(argv[i + 1]);
            } 
            else if (arg == "--all") 
            {
                params.save_all = true;
            }
            else if (arg == "--display") 
            {
                params.do_display_board = true;
            }
        }

        if (params.input_file.empty() || params.iterations <= 0) 
        {
            printError("Invalid input file name or incorrect iteration number specification.");

            params.init();    
            return false;
        }

        return true;
    }

    //
    // Returns a file name so that: out = /path/to/input/basefilname_iteration.ext
    static std::string getOutputFileName(const std::string &intput_filename, int iteration) 
    {
        std::filesystem::path path(intput_filename);
        std::filesystem::path filepath = path.parent_path();
        std::string filename_without_extension = path.stem().string();

        std::string output_filename = filepath.string() + "\\" + filename_without_extension;
        output_filename = output_filename 
                            + "_" + std::to_string(iteration) 
                            + path.extension().string();

        return output_filename;
    }
};

// ----------------------------------------------------------------------------

class Board 
{
 private:
    char ALIVE_CHAR = '*';
    char DEAD_CHAR = '_';

    int width = 0;
    int height = 0;
    std::vector<std::vector<bool>> board;

    //
    void init() 
    {  
        width = height = 0;
        board.clear();
    }

    void resizeBoard()
    {
        board.resize(height, std::vector<bool>(width, false));
    }

    void resize(int w, int h)
    {
        width = w;
        height = h;
        resizeBoard();
    }

    // Getters / Setters...
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool getCell(int x, int y) const { return board[y][x]; }
    void setCell(int x, int y, bool alive) { board[y][x] = alive; }

    // How many neighbours for cell (x, y)?
    int countNeighbors(int x, int y) const 
    {
        int count = 0;
        for (int i = -1; i <= 1; ++i) 
        {
            for (int j = -1; j <= 1; ++j) 
            {
                int newX = (x + i + width) % width;
                int newY = (y + j + height) % height;
                if (board[newY][newX]) 
                {
                    count++;
                }
            }
        }

        if (board[y][x])
        {
            count--; // We donot account for the cell itself.
        }

        return count;
    }

    // Testing...
    bool isValidChar(char ch) 
    {
        return ch == ALIVE_CHAR || ch == DEAD_CHAR;
    }

    bool isValidInputText(const std::vector<std::string> &string_vect) 
    {
        for (const std::string& row : string_vect) 
        {
            if (row.size() != string_vect[0].size() || std::any_of(row.begin(), row.end(), 
                                                                    [&](char ch) { return !isValidChar(ch); }))
            {
                return false;
            }
        }
        return true;
    }

 public:
    Board() = default;

    // 
    bool isValid() { return (width == height && width > 0); }

    // Read file...
    bool read(const std::string &filename) 
    {
        std::vector<std::string> string_vect;

        std::ifstream file(filename);
        if (file) 
        {
            std::string line;
            while (getline(file, line)) 
            {
                string_vect.push_back(line);
            }

            file.close();
        }

        if (string_vect.empty()) 
        {
            IO_Utils::printError("Unable to read input file.");
            return false;
        }

        if (!isValidInputText(string_vect)) 
        {
            IO_Utils::printError("Invalid characters or incompatible row lengths in the input file.");
            return false;
        }

        height = string_vect.size();
        width = string_vect[0].size();
        resizeBoard();

        for (int y=0; y<height; ++y) 
        {
            for (int x=0; x<width; ++x) 
            {
                setCell(x, y, (string_vect[y][x] == ALIVE_CHAR));
            }
        }

        return true;
    }

    // Save board...
    bool save(const std::string &filename)
    {
        std::ofstream file(filename);
        if (!file) 
        {
            IO_Utils::printError("Unable to open output file.");
            return false;
        }

        for (int y = 0; y < height; ++y) 
        {
            for (int x = 0; x < width; ++x) 
            {
                file << (getCell(x, y) ? ALIVE_CHAR : DEAD_CHAR);
            }
            file << "\n";
        }

        file.close();
        return true;       
    }

    // Function to print the board to CLI...
    void displayBoard() 
    {
        system("cls");

        for (int y = 0; y < height; ++y) 
        {
            for (int x = 0; x < width; ++x) 
            {
                if (board[y][x]) 
                {
                    std::cout << ALIVE_CHAR;
                } 
                else 
                {
                    std::cout << DEAD_CHAR;
                }
            }
            std::cout << "\n";
        }
    }

    //
    void update() 
    {
        std::vector<std::vector<bool>> new_board = board;
        for (int y = 0; y < height; ++y) 
        {
            for (int x = 0; x < width; ++x) 
            {
                int neighbors = countNeighbors(x, y);
                if (board[y][x]) 
                {
                    new_board[y][x] = (neighbors == 2 || neighbors == 3);
                } 
                else 
                {
                    new_board[y][x] = (neighbors == 3);
                }
            }
        }
        board = new_board;
    }
};

// ----------------------------------------------------------------------------

int main(int argc, char* argv[]) 
{
    Params params;

    // Parse input arguments...
    if (!IO_Utils::parseProgramArguments(argc, argv, params)) 
    {
        IO_Utils::printUsage();
        return 1; 
    }

    // Read input file and ininitialize the board (file)...
    // --> Testing (input file, valididy, ...) is done inside.
    Board board;
    if (!board.read(params.input_file))
    {
        return 1;
    }

    // Simulate and output...
    for (int i=1; i<=params.iterations; ++i) 
    {
        board.update();

        std::string output_filename = "";
        if (params.save_all || i==params.iterations)
        {
            output_filename = IO_Utils::getOutputFileName(params.input_file, i);
            board.save(output_filename);
        }

        if (!params.do_display_board)
        {
            IO_Utils::printStatus(i, output_filename);
        }
        else 
        {
            board.displayBoard();
            std::this_thread::sleep_for(std::chrono::milliseconds(650));
        }
    }

    // 
    IO_Utils::printSuccess();

    return 0;
}
