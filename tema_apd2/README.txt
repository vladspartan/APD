Tema 2 APD

Student: NICULA Bogdan Daniel
Grupa:	334CB

0.1) Despre structura proiectului:

nume surse: My_Worker.java, WorkPool.java, Main.java
alte fisiere: Makefile, README.txt

Sursa principala este Main.java.

Makefile-ul are o regula pentru compilare: build
Si una pentru curatare: clean

Pentru a rula tema, folositi comanda(dupa compilare):

java Main $num_threads $in_file $out_file

exemplu:

java Main 5 "input.txt" "out.txt"




0.2)Alte specificatii:

S-a folosit scheletul laboratorului 5(Replicated Workers).


OBSERVATIE 1

Avem 3 tipuri de PartialSolution (de task-uri):

->tip 1: PartialSolution(String f, int offset, int length, boolean isEnd)
Worker-ul primeste un nume de fisier, un offset pentru inceputul fragmentului analizat
dimensiunea fragmentului si un indicator pentru final document(pentru a evita depasirea spatiului alocat).

->tip2: PartialSolution(int size, TreeMap<String, TreeMap<String, Integer>> sol)
Worker-ul primeste numarul de chunk-uri(fragmente) din care a fost alcatuita solutia partiala
si solutia partiala.

->tip3: PartialSolution(String f1, String f2, int wc1, int wc2)
Worker-ul primeste numele celor 2 fisiere pentru care verificam similaritatea si numarul de
cuvinte din fiecare.


OBSERVATIE 2
Dupa executarea task-urilor Map-Reduce vectorul de workeri din main este golit,
se initializeaza noi workeri care se vor ocupa de calcularea gradelor de similaritate.

OBSERVATIE 3
Orice caracter exceptand "'" si caracterele alfanumerice(a-zA-Z0-9) este considerat ca separator.
Datorita acestui fapt rezultatele obtinute vor diferi fata de cele prezentate pe site.
(unde separatori erau considerati doar \n \t \r " ")
Spre deosebire de exemplul de pe site documentul 4 pare a fi mai apropiat de doc1 decat 
documentul 3(diferenta mica, de ordin 10^-3). 


OBSERVATIE 4
Programul a fost testat cu chunksize-uri intre 50 si 1000 si a functionat corect.



1.) Implementare

Am gandit procesul ca fiind compus din 3 pasi, toti putand fi executati in paralel:


----------------
->Pasul 1(Map)##
----------------
Toate fisierele sunt impartite in bucati de dimensiune "chunksize".
Aceasta parte este realizata la nivelul metodei main(din clasa Main) prin intermediul metodei statice addWork_Map.
Aceasta adauga in WorkPool task-uri de tip 1(extragere cuvinte dintr-un fragment de text).

La preluarea unui astfel de task, un Worker va extrage exact fragmentul dorit din fisier(metoda readFile)
si apoi va genera un Map al numarului de aparitii pentru fiecare cuvant(TreeMap<String,Integer>), folosind metoda 
numWords.


In urma acestei prelucrari au rezultat solutii partiale(map-uri care contorizeaza numarul de aparitii al 
cuvintelor intr-un fragment). Pentru a putea combina mai usor rezultatele la pasul urmator 
map-urile vor fi de tipul:

TreeMap<String, TreeMap<String, Integer>>
	nume_fisier	cuvant	nr_aparitii

Pentru fiecare task de tip 1 primit se va genera 1 task de tip 2 continand un map, asemenea celui de mai sus,
si o dimensiune(considerata mereu 1).


-------------------
->Pasul 2(Reduce)##
-------------------
Consta in combinarea solutiilor partiale obtinute la pasul anterior si generarea unei solutii finale.

Se primesc 2 task-uri de tip 2 si se genereaza un singur task de tip 2.(Pana ce ramane unul singur)

Un Worker va primi un task, ale carui informatii le va retine local(in variabilele aux si aux_s)
si il va combina cu urmatorul task primit, punand rezultatul in coada de task-uri, si reinitializand aux si aux_s.

In cazul in care coada este goala si un worker are un task de tip 2(nefinalizat) deja preluat acesta va fi repus
in coada de task-uri(metoda freePartialSolution()).

Combinarea a 2 task-uri consta in "concatenarea" celor 2 map-uri si insumarea dimensiunilor.
La analizarea unui task nou verificam daca dimensiunea solutiei partiale nu este egala cu dimensiunea solutiei finale.
Daca cele 2 sunt egale inseamna ca am obtinut rezultatul dorit si ne putem opri.

Solutia finala se salveaza intr-un camp static al clasei My_Worker.



-------------------------
->Pasul 3(Similaritate)##
-------------------------
La sfarsitul pasului 2 workerii si-au incheiat activitatea, iar map-ul de numar aparitii cuvinte
se afla salvat in campul my_word_map.

La nivelul metodei main vom genera noi task-uri pentru calculul similaritatii.(task-uri de tip 3)
Acestea vor specifica numele celor 2 documente analizate si numarul total de cuvinte continut de fiecare.
Se va calcula un grad de similaritate, care se salveaza in structura statica my_similarity_map din clasa My_Worker.

Cand toti workerii isi vor incheia activitatea, structura my_similarity_map va contine grade de similaritate 
calculate intre fisierul analizat si toate celelalte fisiere enumerate.

Map-ul(TreeMap<String,Float>) continand asocierile nume_fisier - grad_similaritate va fi sortat descrescator
si se trec in fisierul de iesire rezultatele relevante(cu grad de similaritate mai mare decat cel minim impus).


