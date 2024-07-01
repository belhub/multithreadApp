// jesli sa jakiekolwiek punkty po prawej stronie na pionowym torze(niebieskim),
// punkty z poziomego toru(czerwonego)jesli znajduja sie przed prawa strona toru czerwonego (na gornym lub dolnym poziomie),
// czekaja az wszystkie punkty na niebieskim torze opuszcza prawa strone toru
#include <iostream>
#include "./glad.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>
#include <vector>
#include <list>
#include <random>
#include <condition_variable>
using namespace std::chrono_literals;
static void error_callback(int error, const char *description)
{
    fprintf(stderr, "Error: %s\n", description);
}
bool shouldTerminate = true;
int counter = 0;
bool go_back = false;
bool go_down = false;
bool go_up = false;
int onRightSideCounter = 3;

// Struktura przechowująca pozycję punktow poziomych
struct PointHorizontal
{
    float x;
    float y;
    int speed;
    float r;
    float g;
    float b;
    bool go_back;
    bool go_down;
    bool go_up;
    int counter;
    bool isActive;
    bool stop_thread;
};

bool shouldCloseWindow = false;
bool endOfThreads = false;
std::list<PointHorizontal> pointsHorizontal;
std::vector<std::thread> threadsHorizontal;

std::mutex mtx;
std::mutex newPointMTX;
std::mutex closeMTX;
bool onRightSide = false;
bool stop_thread = false;
std::condition_variable cv;

bool stopSecondVector = false;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        std::unique_lock<std::mutex> shouldLock(closeMTX);
        shouldCloseWindow = true;
        shouldLock.unlock();

        std::unique_lock<std::mutex> lock(mtx);
        onRightSide = false;
        cv.notify_all();
        lock.unlock();
        std::cout << "Space" << std::endl; // notify onrightside i czekanie czy nie jest watek konczony

        for (size_t i = 0; i < threadsHorizontal.size(); i++)
        {
            // Poczekaj na zakończenie wątku
            if (threadsHorizontal[i].joinable())
            {
                threadsHorizontal[i].join(); // zwolniona pamiec zanim join sie do niego odwoluje - zla kolejnosc joinow prawdopodobnie
            }
        }
    }
}

// Struktura przechowująca pozycję punktow pionowych
struct PointPerpendiculary
{
    float x;
    float y;
    float r;
    float g;
    float b;
    bool go_back;
    bool go_down;
    bool go_up;
    int counter;
};

// Funkcja poruszająca punktem w poziomie
void movePointHorizontal(PointHorizontal *point)
{
    while (point->isActive) // zakonczenie zrobieniem 3 okrazen
    {
        std::chrono::milliseconds duration(point->speed);
        std::this_thread::sleep_for(duration);

        std::unique_lock<std::mutex> shouldLock(closeMTX);
        if (shouldCloseWindow) // zakonczenie spacja
        {
            break;
        }
        shouldLock.unlock();

        float yR = std::round(point->y * 100.0f) / 100.0f;
        float xR = std::round(point->x * 100.0f) / 100.0f;

        if ((yR == 0.2f && xR == 0.1f) || (yR == -0.2f && xR == 0.3f))
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, []
                    { return !onRightSide; });
            std::cout << "Thread resumed" << std::endl;
        }

        if (xR == 0.9f)
        {
            point->go_back = true;
            point->go_down = true;
        }
        if (xR == -0.9f)
        {
            point->go_back = false;
            point->go_up = true;
        }
        if (yR == -0.2f)
        {
            point->go_down = false;
            if (!(point->go_down) && !(point->go_back))
            { // zrobi tak, zeby kazda funkcja znikala po 3 okrazeniach
                point->counter += 1;
            }
        }
        if (yR == 0.2f)
        {
            point->go_up = false;
        }
        if (point->go_down)
        {
            point->y -= (0.01f);
        }
        if (point->go_up)
        {
            point->y += (0.01f);
        }
        if (!(point->go_down || point->go_up))
        {
            if (point->go_back)
            {
                point->x -= (0.01f);
            }
            else
            {
                point->x += (0.01f);
            }
        }
        if (point->counter >= 3)
        {
            point->x -= (0.01f);
        }
        if (xR < -1.0f)
        {
            point->isActive = false;
        }
    }
}

// Funkcja poruszająca punktem w pionie
void movePointPerpendiculary(
    float *x, float *y,
    bool *go_back, bool *go_down, bool *go_up,
    float *r, float *g, float *b, int *counter)
{
    while (true)
    {
        std::this_thread::sleep_for(50ms);

        std::unique_lock<std::mutex> shouldLock(closeMTX);
        if (shouldCloseWindow) // zakonczenie spacja
        {
            break;
        }
        shouldLock.unlock();

        float yR = std::round(*y * 100.0f) / 100.0f;
        float xR = std::round(*x * 100.0f) / 100.0f;

        if (xR == 0.2f && yR == 0.9f)
        { // jesli punkt wchodzi w prawej osi pionowej to dodajemy do vectora jego "obecnosc"
            onRightSideCounter++;
        }
        if (xR == 0.2f && yR == -0.9f)
        { // jesli punkt wychodzi w prawej osi pionowej to odejmujemy z vectora jego "obecnosc"
            onRightSideCounter--;
        }

        if (xR == 0.2f)
        {
            *go_back = true;
            *go_down = true;
        }
        if (xR == -0.2f)
        {
            *go_back = false;
            *go_up = true;
        }
        if (yR == -0.9f)
        {
            *go_down = false;
            *go_back = true;
        }
        if (yR == 0.9f)
        {
            *go_up = false;
            *go_back = false;
        }
        if (*go_down)
        {
            *y -= 0.01f;
        }
        if (*go_up)
        {
            *y += 0.01f;
        }
        if (!(*go_down || *go_up))
        {
            if (*go_back)
            {
                *x -= 0.01f;
            }
            else
            {
                *x += 0.01f;
            }
        }
        std::unique_lock<std::mutex> lock(mtx);

        if (onRightSideCounter > 0)
        {
            onRightSide = true;
            // cv.notify_all();
        }
        else
        {
            onRightSide = false; // tyko tutaj notify
            cv.notify_all();
        }
    }
}
// Funkcja rysująca kwadrat
void drawSquare(float r, float g, float b, bool rotation, bool inside, bool completePath)
{
    // Współrzędne wierzchołków prostokata
    GLfloat verticesHor[] = {
        -0.95f, -0.25f, 0.0f, // lewy dolny
        0.95f, -0.25f, 0.0f,  // prawy dolny
        0.95f, 0.25f, 0.0f,   // prawy górny
        -0.95f, 0.25f, 0.0f   // lewy górny
    };
    GLfloat verticesHorIn[] = {
        -0.85f, -0.15f, 0.0f, // lewy dolny
        0.85f, -0.15f, 0.0f,  // prawy dolny
        0.85f, 0.15f, 0.0f,   // prawy górny
        -0.85f, 0.15f, 0.0f   // lewy górny
    };
    GLfloat verticesHor_up[] = {
        -0.15f, 0.15f, 0.0f, // lewy dolny
        0.15f, 0.15f, 0.0f,  // prawy dolny
        0.15f, 0.25f, 0.0f,  // prawy górny
        -0.15f, 0.25f, 0.0f  // lewy górny
    };
    GLfloat verticesHor_down[] = {
        -0.15f, -0.25f, 0.0f, // lewy dolny
        0.15f, -0.25f, 0.0f,  // prawy dolny
        0.15f, -0.15f, 0.0f,  // prawy górny
        -0.15f, -0.15f, 0.0f  // lewy górny
    };

    // Współrzędne wierzchołków prostokata
    GLfloat verticesPar[] = {
        -0.25f, -0.95f, 0.0f, // lewy dolny
        0.25f, -0.95f, 0.0f,  // prawy dolny
        0.25f, 0.95f, 0.0f,   // prawy górny
        -0.25f, 0.95f, 0.0f   // lewy górny
    };
    GLfloat verticesParIn[] = {
        -0.15f, -0.85f, 0.0f, // lewy dolny
        0.15f, -0.85f, 0.0f,  // prawy dolny
        0.15f, 0.85f, 0.0f,   // prawy górny
        -0.15f, 0.85f, 0.0f   // lewy górny
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    if (!completePath)
    {
        if (rotation && !inside)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesHor), verticesHor, GL_STATIC_DRAW);
        }
        else if (!rotation && !inside)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPar), verticesPar, GL_STATIC_DRAW);
        }
        else if (rotation && inside)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesHorIn), verticesHorIn, GL_STATIC_DRAW);
        }
        else if (!rotation && inside)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesParIn), verticesParIn, GL_STATIC_DRAW);
        }
    }
    else
    {
        if (rotation)
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesHor_up), verticesHor_up, GL_STATIC_DRAW);
        }
        else
        {
            glBufferData(GL_ARRAY_BUFFER, sizeof(verticesHor_down), verticesHor_down, GL_STATIC_DRAW);
        }
    }

    // Pozycja
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Ustawienia koloru
    if (!inside)
    {
        glColor4f(r, g, b, 0.2f); // kolor
    }
    else
    {
        glColor3f(r, g, b); // kolor
    }

    // Włączenie przezroczystości
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Rysowanie kwadratu
    glBindVertexArray(VAO);
    glDrawArrays(GL_QUADS, 0, 4);
    glBindVertexArray(0);
    // Wyłączenie przezroczystości
    glDisable(GL_BLEND);
    // Czyszczenie
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
void drawPath()
{
    drawSquare(1.0f, 0.0f, 0.0f, true, false, false);
    drawSquare(0.25f, 0.25f, 0.25f, true, true, false);
    drawSquare(0.0f, 0.0f, 1.0f, false, false, false);
    drawSquare(0.25f, 0.25f, 0.25f, false, true, false);
    drawSquare(1.0f, 0.0f, 0.0f, true, false, true);
    drawSquare(1.0f, 0.0f, 0.0f, false, false, true);
}
float randomFloat(float min, float max)
{
    // Utwórz generator liczb losowych
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    float randomValue = dis(gen);
    // Wygeneruj liczbę losową z rozkładu jednorodnego w podanym zakresie

    // Zaokrąglij liczbę do zadanej liczby miejsc po przecinku
    float factor = std::pow(10.0f, 1);
    randomValue = std::round(randomValue * factor) / factor;
    return dis(gen);
}

int getRandomInt(int min, int max)
{
    std::random_device rd;                         // używamy urządzenia losującego
    std::mt19937 gen(rd());                        // generator Mersenne Twister
    std::uniform_int_distribution<> dis(min, max); // dystrybucja równomierna
    // Wygeneruj liczbę losową z rozkładu jednorodnego w podanym zakresie
    return dis(gen); // zwraca losową liczbę całkowitą z zakresu [min, max]
}

void insertThread()
{
    std::lock_guard<std::mutex> newPointLock(newPointMTX);
    PointHorizontal newPoint = {-0.9f, 0.2f, getRandomInt(5, 50), randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f), false, false, false, 0, true, false};
    pointsHorizontal.push_back(newPoint);
    PointHorizontal *point = &pointsHorizontal.back();
    threadsHorizontal.emplace_back(movePointHorizontal, point); // problem - jesli dodaje pojedynczo poprzedni watek sie zatrzymuje
}

void randomInsertion()
{
    while (true)
    {
        std::unique_lock<std::mutex> shouldLock(closeMTX);
        if (shouldCloseWindow) // zakonczenie spacja
        {
            break;
        }
        shouldLock.unlock();
        insertThread();
        std::chrono::seconds duration(getRandomInt(2, 5));
        std::this_thread::sleep_for(std::chrono::seconds(duration)); // Losowy czas od 2 do 10 sekund
    }
}

int main(int, char **)
{
    GLFWwindow *window;

    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwSetErrorCallback(error_callback);

    window = glfwCreateWindow(640, 480, "Window", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Couldn't load opengl" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetKeyCallback(window, key_callback);

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

    // Główna pętla
    std::vector<PointPerpendiculary> pointsPerpendiculary;
    pointsPerpendiculary.push_back({0.20f, randomFloat(-0.9f, 0.9f), 0.75f, 0.0f, 0.0f, false, false, false, 0});
    pointsPerpendiculary.push_back({0.20f, randomFloat(-0.9f, 0.9f), 0.0f, 0.75f, 0.0f, false, false, false, 0});
    pointsPerpendiculary.push_back({0.20f, randomFloat(-0.9f, 0.9f), 0.0f, 0.0f, 0.75f, false, false, false, 0});

    glPointSize(10.0f);

    std::vector<std::thread> threadsPerpendiculary;
    for (auto &point : pointsPerpendiculary)
    {
        threadsPerpendiculary.emplace_back(movePointPerpendiculary,
                                           &point.x, &point.y,
                                           &point.go_back, &point.go_down, &point.go_up,
                                           &point.r, &point.g, &point.b, &point.counter);
    }

    // watek dodajcy kolejne watki punkty do pointsHorizontal
    std::thread addPointThread(randomInsertion);

    bool close = true;

    while (!glfwWindowShouldClose(window))
    {
        // Czyszczenie bufora koloru i głębokości
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Rysowanie ramki prostokąta
        drawPath();

        for (auto &point : pointsPerpendiculary)
        {
            glBegin(GL_POINTS);
            glColor3f(point.r, point.g, point.b);
            glVertex2f(point.x, point.y);
            glEnd();
        }

        for (auto &point : pointsHorizontal)
        {
            // Narysuj punkt na aktualnej pozycji
            glBegin(GL_POINTS);
            glColor3f(point.r, point.g, point.b);
            glVertex2f(point.x, point.y);
            glEnd();
        }

        // Sprawdź i obsłuż zdarzenia oraz wymień bufory
        glfwSwapBuffers(window);
        glfwPollEvents();

        std::unique_lock<std::mutex> shouldLock(closeMTX);
        if (shouldCloseWindow) // zakonczenie spacja
        {
            break;
        }
        shouldLock.unlock();
    }

    std::cout << std::endl
              << "STOP";

    for (auto &thread : threadsPerpendiculary)
    {
        thread.join();
    }
    std::cout << std::endl
              << "STOP threadsPerpendiculary";

    if (addPointThread.joinable())
    {
        addPointThread.join();
        std::cout << std::endl
                  << "STOP addPointThread";
    }

    glfwTerminate();
    return 0;
}