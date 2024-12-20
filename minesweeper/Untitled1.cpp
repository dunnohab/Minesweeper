#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <string>

int TILE_SIZE = 40;
int HEADER_HEIGHT = 40;
int hintsLeft = 3;
int GRID_WIDTH;
int GRID_HEIGHT;
int BOMB_COUNT;

SDL_Color TILE_COLOR = {169, 169, 169, 255};
SDL_Color REVEALED_COLOR = {200, 200, 200, 255};
SDL_Color FLAG_COLOR = {255, 0, 0, 255};
SDL_Color BOMB_COLOR = {255, 0, 0, 255};
SDL_Color TEXT_COLOR = {0, 0, 0, 255};

struct Tile {
    bool isBomb = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int surroundingBombs = 0;
};

std::vector<std::vector<Tile>> grid;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
bool gameOver = false;
bool gameWon = false;
int flagsLeft;

SDL_Rect hintButtonRect;

void drawText(const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, nullptr, &destRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void initializeGrid() {
    srand(static_cast<unsigned int>(time(0)));
    grid.resize(GRID_HEIGHT, std::vector<Tile>(GRID_WIDTH));
    flagsLeft = BOMB_COUNT;
    gameOver = false;
    gameWon = false;

    for (auto& row : grid) {
        for (auto& tile : row) {
            tile = Tile();
        }
    }

    int bombsPlaced = 0;
    while (bombsPlaced < BOMB_COUNT) {
        int x = rand() % GRID_WIDTH;
        int y = rand() % GRID_HEIGHT;
        if (!grid[y][x].isBomb) {
            grid[y][x].isBomb = true;
            bombsPlaced++;
        }
    }

    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x].isBomb) continue;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < GRID_WIDTH && ny < GRID_HEIGHT && grid[ny][nx].isBomb) {
                        grid[y][x].surroundingBombs++;
                    }
                }
            }
        }
    }
}

void revealTile(int x, int y) {
    if (x < 0 || y < 0 || x >= GRID_WIDTH || y >= GRID_HEIGHT || grid[y][x].isRevealed || grid[y][x].isFlagged) return;
    grid[y][x].isRevealed = true;
    if (grid[y][x].isBomb) {
        gameOver = true;
    } else if (grid[y][x].surroundingBombs == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                revealTile(x + dx, y + dy);
            }
        }
    }
}

bool checkWinCondition() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (!grid[y][x].isBomb && !grid[y][x].isRevealed) {
                return false;
            }
        }
    }
    return true;
}

void provideHint() {
    if (hintsLeft > 0) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            for (int x = 0; x < GRID_WIDTH; x++) {
                if (!grid[y][x].isRevealed && !grid[y][x].isBomb && !grid[y][x].isFlagged) {
                    revealTile(x, y);
                    hintsLeft--;
                    return;
                }
            }
        }
    } else {
        std::cout << "No hints left!" << std::endl;
    }
}


void handleMouseClick(int x, int y, bool isRightClick) {
    if (gameOver || gameWon) return;

    if (!isRightClick && x >= hintButtonRect.x && x <= hintButtonRect.x + hintButtonRect.w &&
        y >= hintButtonRect.y && y <= hintButtonRect.y + hintButtonRect.h) {
        provideHint();
        return;
    }

    int gridX = x / TILE_SIZE;
    int gridY = (y - HEADER_HEIGHT) / TILE_SIZE;

    if (gridX < 0 || gridX >= GRID_WIDTH || gridY < 0 || gridY >= GRID_HEIGHT) return;

    Tile& tile = grid[gridY][gridX];
    if (isRightClick) {
        if (!tile.isRevealed) {
            tile.isFlagged = !tile.isFlagged;
            flagsLeft += tile.isFlagged ? -1 : 1;
        }
    } else {
        if (!tile.isFlagged && !tile.isRevealed) {
            revealTile(gridX, gridY);
            gameWon = checkWinCondition();
        }
    }
}


void drawGrid() {
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            Tile& tile = grid[y][x];
            SDL_Rect tileRect = {x * TILE_SIZE, HEADER_HEIGHT + y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_Color fillColor = tile.isRevealed ? REVEALED_COLOR : TILE_COLOR;
            SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
            SDL_RenderFillRect(renderer, &tileRect);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &tileRect);

            if (tile.isRevealed && tile.isBomb) {
                drawText("B", tileRect.x + 10, tileRect.y + 10, BOMB_COLOR);
            } else if (tile.isRevealed && tile.surroundingBombs > 0) {
                drawText(std::to_string(tile.surroundingBombs), tileRect.x + 10, tileRect.y + 10, TEXT_COLOR);
            } else if (tile.isFlagged) {
                drawText("F", tileRect.x + 10, tileRect.y + 10, FLAG_COLOR);
            }
        }
    }
}

void drawHeader() {
    drawText("Flags left: " + std::to_string(flagsLeft), 10, 10, TEXT_COLOR);

    SDL_SetRenderDrawColor(renderer, 100, 100, 250, 255); // Light blue color for the button
    SDL_RenderFillRect(renderer, &hintButtonRect);
    drawText("Hint (" + std::to_string(hintsLeft) + ")", hintButtonRect.x + 10, hintButtonRect.y + 2, TEXT_COLOR);

    if (gameOver) {
        drawText("You Lose!", GRID_WIDTH * TILE_SIZE - 100, 10, BOMB_COLOR);
    } else if (gameWon) {
        drawText("You Win!", GRID_WIDTH * TILE_SIZE - 100, 10, FLAG_COLOR);
    }
}


int main(int argc, char* args[]) {
    std::cout << "Enter grid width: ";
    std::cin >> GRID_WIDTH;
    std::cout << "Enter grid height: ";
    std::cin >> GRID_HEIGHT;
    std::cout << "Enter bomb count: ";
    std::cin >> BOMB_COUNT;

    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        std::cerr << "Failed to initialize SDL or SDL_ttf: " << SDL_GetError() << "\n";
        return 1;
    }

    int SCREEN_WIDTH = GRID_WIDTH * TILE_SIZE;
    int SCREEN_HEIGHT = GRID_HEIGHT * TILE_SIZE + HEADER_HEIGHT;

    window = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    font = TTF_OpenFont("arial.ttf", 20);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }


    hintButtonRect = {SCREEN_WIDTH / 2 - 50, 10, 100, 20};

    initializeGrid();

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x;
                int y = event.button.y;
                handleMouseClick(x, y, event.button.button == SDL_BUTTON_RIGHT);
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        drawHeader();
        drawGrid();
        SDL_RenderPresent(renderer);

        if (gameOver || gameWon) {
            SDL_Delay(10000);
            break;
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_Quit();

    return 0;
}















