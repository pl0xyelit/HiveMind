# Ce trebuie sa invat sa fac:
## Patternuri:
* Singleton (La ce naiba il folosesc conform cerintei?)
* Factory (Probabil creeaza roboti/drone/scutere)
* Strategy (o clasa de strategii de generare de Maps)
    * Strategia 1: genereaza harta pe baza unui fisier text, deja dat (pentru debug, harta nu trebuie hardcodata)
    * Strategia 2: genereaza harta pe baza unui noisemap, procedural
## Algoritmi
* Dispatch (trebuie sa eficientizez algoritmul pentru profit)
* Noisemap procedural generation pentru harta hahahahah
## Error Handling 
* Folosesc o abordare rudimentara unde dau return prematur sau returnez o valoare numerica care inseamna ca ceva a mers prost, trebuie sa implementez erori si sa fac handling al erorilor, in caz ca se strica ceva la mijlocul executiei...




# Ce am facut deja:
## Algoritmi
* Sistem bazat pe tick-uri (gen minecraft hahahahahah)
* Dispatch (crearea unei logici care ia diverse actiuni la fiecare tick)
## Design patterns
* Factory:
    * Nu cred ca am implementat patternul, dar robotii sunt creati
* Strategy (doar ca NU e implementat cum trebuie modelul Strategy, NU am o interfata IMapGenerator pentru asta)
    * Strategia 1: genereaza harta pe baza unui fisier text, deja dat (pentru debug, harta nu trebuie hardcodata)
    * Strategia 2: genereaza harta pe baza unui noisemap, procedural

# Structura: ce am nevoie:
## Clase
* Courier
    * clasa abstracta, va avea campurile
        * viteza
        * baterie
        * consum/tick
        * cantitate pachete
        * virtual move() (drona va avea implementare diferita a acestei metode intrucat aceasta poate survola deasupra zidurilor)
    * legenda:
        * (^) - drone (poate zbura peste ziduri)
        * (R) - robot (cel mai ieftin, lent, incapator)
        * (S) - scuter (middle ground)
* Map
    * stocheaza o matrice MxN, fiecare element fiind un spatiu/celula pe care se afla ceva:
        * (.) - drum (orice vehicul poate calatori aici)
        * (#) - zid (doar drona poate trece peste el)
        * (D) - destinatie (un loc unde un anumit pachet trebuie sa ajunga)
        * (S) - statie de incarcare, +25% baterie/tick
        * (B) - baza (doar una)
            * punct de plecare/spawnare a Courierilor
            * locul de unde este preluat fiecare pachet, cu scopul de a fi trimis
            * stationarea in Baza produce aceiasi incarcare ca o statie de incarcare