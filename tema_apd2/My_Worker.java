import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.TreeMap;

/**
 * Clasa ce reprezinta o solutie partiala pentru problema de rezolvat. Aceste
 * solutii partiale constituie task-uri care sunt introduse in workpool.
 */
class PartialSolution {
	
	int type;/* 1 pentru map, 2 pentru reduce, 3 pentru calcul similaritate */
	
	/* Variabile pentru tipul 1(Map) */
	String filename;
	int offset, length;
	TreeMap<String, TreeMap<String, Integer>> partialMap;
	boolean isEnd;
	
	/* Variabile pentru tipul 2(Reduce) */
	int size;
	
	/* Variabile pentru tipul 3 - calcul similaritate */
	String f1, f2;
	int wc1, wc2;
	
	/* Constructor solutii partiale pentru pasul de Map */
	public PartialSolution(String f, int offset, int length, boolean isEnd) {
		
		filename = f;
		this.offset = offset;
		this.length = length;
		this.isEnd = isEnd;
		partialMap = new TreeMap<String, TreeMap<String, Integer>>();
		
		type = 1;
	}
	
	/* Constructor solutii partiale pentru pasul de Reduce */
	public PartialSolution(int size, TreeMap<String, TreeMap<String, Integer>> sol) {
		
		this.size = size;
		partialMap = sol;
		
		type = 2;
	}
	
	
	/* Constructor solutii partiale pentru pasul de calcul similaritate */
	public PartialSolution(String f1, String f2, int wc1, int wc2)
	{
		this.f1 = f1;
		this.f2 = f2;
		this.wc1 = wc1;
		this.wc2 = wc2;
		
		type = 3;
	}
	
	
	public String toString() {
		if(type == 1)
			return filename + " " + offset + " " + length;
		return f1 + " " + f2 + " " + wc1 + " " + wc2;
	}
}

/**
 * Clasa ce reprezinta un thread worker.
 */
public class My_Worker extends Thread {
	WorkPool wp;
	static TreeMap<String,TreeMap<String,Integer>> my_word_map;/* harta nr aparitii cuvinte pentru fiecare fisier */
	static TreeMap<String, Float> my_similarity_map;/* harta grad similaritate pentru fiecare fisier*/
	static int finalSolSize = 0;/* dimensiune solutie finala */
	TreeMap<String, TreeMap<String, Integer>> aux = null;/* map auxiliar pentru reduce */
	int aux_s;
	
	
	public My_Worker(WorkPool workpool) {
		if(my_word_map == null)
			my_word_map = new TreeMap<String,TreeMap<String,Integer>>();
		if(my_similarity_map == null)
			my_similarity_map = new TreeMap<String,Float>();
		
		this.wp = workpool;
	}

	
	/**
	 * Metoda care intoarce un HashMap continand nr de aparitii al fiecarui token.
	 * Textul primit ca parametru se presupune ca este "curat"
	 * (contine doar caractere alfanumerice spatii si apostroafe).
	 * 
	 */
	public static TreeMap<String,Integer> numWords(String text)
	{
		TreeMap<String,Integer> result = new TreeMap<String,Integer>();
		String[] words = text.trim().split(" ");
		
		for(String e: words)
		{
			if(e.equals(""))
				continue;
			if(!result.containsKey(e.toLowerCase()))
				result.put(e.toLowerCase(), 1);
			else
			{
				Integer aux = result.get(e.toLowerCase());
				result.remove(e.toLowerCase());
				result.put(e.toLowerCase(), aux.intValue() + 1);
			}
		}
		
		return result;
	}	
	
	
	/**
	 * Metoda care se ocupa cu citirea unui segment din fisier.
	 * 
	 */
	public static String readFile(String filename, int offset, int length, boolean isEnd) throws IOException
	{
		byte[] x = new byte[length + 30];
		/* Deschidem fisier */
		FileInputStream br = new FileInputStream(new File(filename));
		
		/* Daca nu ne aflam chiar la inceputul lui */
		if(offset > 0)
		{
			/* Ne deplasam cu un spatiu inaint de offset */
			br.skip(offset - 1);
			
			br.read(x, 0, 1);
			
			/* Vedem daca de acolo a inceput un cuvant. Daca da, il sarim. */
			if(Character.isLetter(x[0]) || Character.isDigit(x[0]))
				while(Character.isLetter(x[0])|| x[0] == '\'' || Character.isDigit(x[0]))
				{
				
					br.read(x, 0, 1);
					length--;
				}
		}

		if(length == 0)
		{
			br.close();
			return "";
		}
		
		
		/* Citim bytes-ii necesari. */
		br.read(x, 0, length);
		
		/* Daca segmentul se termina cu un cuvant incomplet, extindem segmentul. */
		while(!isEnd && (Character.isLetter(x[length-1])|| x[length - 1] == '\'' || Character.isDigit(x[length - 1])))
		{
			br.read(x, length++, 1);
		}
		
		
		String str = new String(x, 0, --length);
		
		br.close();
		return str;
	}

	
	/**
	 * Metoda care combina 2 TreeMap-uri continand word_count-uri pentru un fisier.
	 * 
	 * @param a
	 * @param b
	 * @return
	 */
	public static TreeMap<String,Integer> combineMaps(TreeMap<String, Integer> a, TreeMap<String, Integer> b)
	{
		for(String e : b.keySet())
		{
			if(!a.containsKey(e))
				a.put(e, b.get(e));
			else
			{
				Integer aux = a.get(e);
				a.remove(e);
				a.put(e, aux.intValue() + b.get(e).intValue());
			}
		}
		
		return a;
	}
	
	
	/**
	 * Metoda care combina 2 TreeMap-uri continand word_count-uri pentru mai multe fisiere.
	 * 
	 * @param a
	 * @param b
	 * @return
	 */
	public static TreeMap<String,TreeMap<String,Integer>> combineAll(TreeMap<String,TreeMap<String, Integer>> a, TreeMap<String,TreeMap<String, Integer>> b)
	{
		for(String e : b.keySet())
		{
			if(!a.containsKey(e))
				a.put(e, b.get(e));
			else
			{
				TreeMap<String,Integer> aux = a.get(e);
				a.remove(e);
				a.put(e, combineMaps(aux, b.get(e)));
			}
		}
		
		return a;
	}
	
	
	/**
	 * Metoda care "elibereaza" o solutie partiala(pentru cazul in care avem numar impar
	 * de solutii partiale si fiecare Worker prelucreaza 2)
	 * 
	 */
	void freePartialSolution()
	{
		wp.putWork(new PartialSolution(aux_s, aux));
		aux = null;
	}
	
	
	/**
	 * Procesarea unei solutii partiale. Aceasta poate implica generarea unor
	 * noi solutii partiale care se adauga in workpool folosind putWork().
	 * Daca s-a ajuns la o solutie finala, aceasta va fi afisata.
	 */
	void processPartialSolution(PartialSolution ps) {
		
		switch(ps.type)
		{
			/* Cazul 1: Map => Construiesc solutii partiale. */
			case 1:
				String result = "";
				try {
					result = readFile(ps.filename, ps.offset, ps.length, ps.isEnd);
				} catch (IOException e) {
					e.printStackTrace();
				}
				
				TreeMap<String, Integer> res_map = numWords(result.replaceAll("[^a-zA-Z0-9' ]", " ").trim());
				TreeMap<String, TreeMap<String, Integer>> final_result = new TreeMap<String, TreeMap<String, Integer>>();
				final_result.put(ps.filename, res_map);/* Map partial de rezultate */
				
				wp.putWork(new PartialSolution(1, final_result));
				
				break;
				
			/* Cazul 2: Reduce => Obtin solutia finala, imbinand solutiile partiale. */
			case 2:
				/* Daca solutia partiala preluata are dimensiunea pe care ar trebui sa o aiba
				 * solutia finala, ne oprim. */
				if(ps.size == finalSolSize)
				{
					synchronized (my_word_map) {
						my_word_map = ps.partialMap;
					}
					break;
				}
				/* Daca nu am preluat nicio solutie partiala, o preiau pe prima. */
				if(aux == null)
				{			
					aux = ps.partialMap;
					aux_s = ps.size;
				}
				/* Daca am preluat deja o solutie, o combin cu cea de-a doua preluata.
				 * Pun rezultatul in coada.*/
				else
				{
					wp.putWork(new PartialSolution(aux_s + ps.size, combineAll(aux, ps.partialMap)));
					aux = null;
				}
				break;
			
			/* Caz 3: Calcul similaritate fisiere. */
			case 3:
				float freq_sum = 0, freq1, freq2; 
				
				for(String word : my_word_map.get(ps.f1).keySet())
				{
					if(my_word_map.get(ps.f2).containsKey(word))
					{
						freq1 = 100f * my_word_map.get(ps.f1).get(word).floatValue() / ps.wc1;
						freq2 = 100f * my_word_map.get(ps.f2).get(word).floatValue() / ps.wc2;
						
						freq_sum += freq1 * freq2 / 100f;
					}
				}
				
				/* Adaug rezultatul la Map-ul de procente de similaritate. */
				synchronized (my_similarity_map) {
					my_similarity_map.put(ps.f2, freq_sum);
				}
				
				
				break;
		}
		
		
	}
	
	public void run() {
		
		System.out.println("Thread-ul worker " + this.getName() + " a pornit...");
		while (true) {
			PartialSolution ps = wp.getWork();
			/* Daca nu mai sunt task-uri de preluat si nu prelucrez nimic momentan. */
			if (ps == null && aux == null)
				break;
			/* Daca nu mai sunt task-uri, dar eu totusi prelucram ceva. */
			else if (ps == null)
			{
				/* Pun inapoi in coada solutia partiala. */
				freePartialSolution();
				continue;
			}
			
			processPartialSolution(ps);
		}
		System.out.println("Thread-ul worker " + this.getName() + " s-a terminat...");
		
	}

	
}

