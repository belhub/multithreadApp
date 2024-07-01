# multithreadApp
prosta aplikacja wielowątkowa oparta na technologiach, C++ oraz OpenGL (biblioteka glfw3 i glad)

Aplikacja ma na celu synchronizację wątków. 
Po rozpoczęciu aplikacji, pojawia się ekran z dwoma torami, na torze pionowym poruszają się trzy piłki o stałej prędkościi losowym punkcie początkowym, na torze poziomym w losowych odstępach czasu pojawiają się nowe piłki o losowej prędkości, piłki na torze poziomym opuszczają tor po 3 okrążeniach. 
Każda piłka poruszająca się po ekranie jest osobnym wątkiem. 
Projekt ma na celu pracę na wątkach(threads) i ich synchronizację przez mutex-y. 
Synchronizacja polega na zatrzymaniu się piłek, na torze poziomym, przed prawą stroną toru pionowego, jeśli jakakolwiek piłka toru pionowego porusza się po tej krawędzi. 
Naciśnięcie klawisza spacji kończy wykonywanie się programu.
