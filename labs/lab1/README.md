**Maze Navigator Challenge**

**Objective:**  
Develop a console-based maze navigation game where the player must find a path from the starting point to the exit. The maze layout is provided to the program via a command-line argument (e.g., a filename containing the maze data). Your task is to implement user input handling, timed responses, hints, and a cheat mode to assist the player.

**Game Overview:**

- **Maze Setup:**  
  - The maze is defined in an external file passed as a command-line argument.
  - The file contains a grid representation where walls, paths, the start point, and the exit are specified.
  - At the start of the game, only the player’s current location is visible; the rest of the maze remains hidden until discovered.

- **Player Movement:**  
  - The player can move using simple commands (for example: `W` for up, `A` for left, `S` for down, and `D` for right).
  - After each move, update and display the maze, revealing the new cell and any adjacent cells that have been "discovered" as a result of the move.

- **Invalid Moves and Hints:**  
  - If the player attempts to move into a wall or outside the maze boundaries, the move is not executed.
  - For each invalid move, the program will automatically reveal one adjacent cell (a “safe” cell) as a hint to help the player progress.

- **Timed Input:**  
  - The program enforces a response time limit of 5 seconds per move.
  - If the player does not input a command within 5 seconds, an alarm (audible alert or on-screen message) is triggered, reminding the player to make a move.

- **Handling Repeated Inputs:**  
  - If the player attempts the same move repeatedly, the maze display is reprinted without changing the game state.
  - Ensure that repeated commands do not negatively affect the game progression or timing.

- **Cheat Mode:**  
  - If the player presses **Ctrl + Z** at any point, the game will reveal one additional hidden section of the maze.
  - This cheat mode should only reveal cells that are part of the safe path, not the entire maze layout.

- **Game Termination:**  
  - The game continues until the player successfully navigates from the start point to the exit.
  - Alternatively, if the maze becomes fully revealed (for instance, after too many incorrect moves or hints), the game will end and display the complete maze layout.

**Implementation Requirements:**

1. **Command-Line Input:**  
   - Your program must accept a maze layout file as a command-line argument.
   - Parse the file to build an internal representation of the maze.

2. **User Interface and I/O:**  
   - Use standard input/output for interaction.
   - Clearly display the maze after every move, highlighting the player's current position and any revealed cells.
   - Provide concise messages for valid moves, invalid moves, hints, and alarms.

3. **Timing Mechanism:**  
   - Implement a timer that starts with each prompt.
   - Trigger an alarm if no input is received within the 5-second window.

4. **Robustness and Error Handling:**  
   - Ensure your program gracefully handles invalid inputs and repeated commands.
   - Validate movement commands and handle edge cases (e.g., moving out of bounds).

5. **Modular Code Structure:**  
   - Organize your code into functions/modules for tasks such as maze parsing, display updates, input handling, timing, and cheat mode activation.
   - Include comments and documentation for clarity.

**Additional Considerations:**

- **Maze Representation:**  
  Decide on an efficient way to represent the maze (e.g., a 2D array or vector) and how you will manage the "hidden" vs. "revealed" cells.

- **User Experience:**  
  Strive to create an intuitive interface that clearly communicates the game state and provides helpful feedback to the player.

- **Testing:**  
  Test your program with various maze layouts and user behaviors to ensure all features (movement, hints, timeouts, and cheat mode) work as intended.
