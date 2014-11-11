import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.Map;
import java.util.TreeMap;


/**
 * Clasa main.
 * 
 * @author Bogdan
 *
 */

public class Main {

	
	/**
	 * Metoda care adauga task-uri(PartialSolutions) pentru pasul de Map. 
	 * 
	 * @param filename
	 * @param chunksize
	 * @param wp
	 */
	public static void addWork_Map(String filename, int chunksize, WorkPool wp)
	{
		int size = (int)(new File(filename)).length();
		

		
		for(int i = 0; i< size / chunksize - 1; i++)
		{
			wp.putWork(new PartialSolution(filename, i * chunksize, chunksize, false));
		}
		
		if(size % chunksize != 0)
		{
			wp.putWork(new PartialSolution(filename, (size / chunksize - 1) * chunksize, chunksize, false));
			wp.putWork(new PartialSolution(filename, (size / chunksize) * chunksize, size - (size / chunksize) * chunksize, true));
			My_Worker.finalSolSize += (size / chunksize) + 1;
		}
		else
		{
			wp.putWork(new PartialSolution(filename, (size / chunksize - 1) * chunksize, chunksize, true));
			My_Worker.finalSolSize += (size / chunksize);
		}
			
		
	}
	
	
	/**
	 * Metoda care adauga task-uri pentru pasul de Reduce.
	 * 
	 * @param my_file
	 * @param files
	 * @param wp
	 */
	public static void addWork_Similarity(String my_file, ArrayList<String> files, WorkPool wp)
	{
		TreeMap<String, Integer> wordsPerDoc = new TreeMap<String, Integer>();
		
		/* Creez o structura in care am numarul de cuvinte din fiecare fisier. */
		calculateWordsPerDoc(wordsPerDoc, My_Worker.my_word_map);
		
		for(int i = 0; i < files.size(); i++)
		{
			wp.putWork(new PartialSolution(my_file, files.get(i), wordsPerDoc.get(my_file), wordsPerDoc.get(files.get(i))));
		}
	}
	
	/**
	 * Metoda care calculeaza numarul de cuvinte din fiecare document.
	 * 
	 * @param dest
	 * Structura in care se vor salva rezultatele.
	 * @param source
	 * Map-ul de aparitii al cuvintelor in fiecare document rezultat in urma pasului de Map.
	 */
	public static void calculateWordsPerDoc(TreeMap<String, Integer> dest, TreeMap<String, TreeMap<String, Integer>> source)
	{
		for(String filename : source.keySet())
		{
			int count = 0;
			for(String word : source.get(filename).keySet())
			{
				count += source.get(filename).get(word).intValue();
			}
			
			dest.put(filename, count);
		}
	}
	
	
	/**
	 * Metoda main.
	 * 
	 * 
	 * @param args
	 * Forma parametrii: nr_thread-uri fisier_in fisier_out
	 * 
	 */
	public static void main(String[] args)
	{
		String file_in = args[1]; /* fisier intrare */
		String file_out = args[2];/* fisier iesire */
		int num_workers = Integer.parseInt(args[0]); /* numar thread-uri */
		
		int chunksize = 0, num_files;
		float accuracy = 0;
		String analized_file = "";/* fisier analizat */
		ArrayList<String> files = new ArrayList<String>();/* lista fisierelor care vor fi citite */
		ArrayList<My_Worker> workers = new ArrayList<My_Worker>();/* vector workeri */
		WorkPool wp;
		
		/* citirea datelor din fisierul de intrare */
		try {
			BufferedReader br = new BufferedReader(new FileReader(new File(file_in)));
			
			analized_file = br.readLine();
			chunksize = Integer.parseInt(br.readLine());
			accuracy = Float.parseFloat(br.readLine());
			num_files = Integer.parseInt(br.readLine());
			
			for(int i = 0; i < num_files; i++)
				files.add(br.readLine());
			
			br.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		
		wp = new WorkPool(num_workers);
		/*--------------------------------------------------------*/
		/* Etapa de Map-Reduce(Pasii 1 si 2)*/
		System.out.println();
		System.out.println("Etapa de calculare a numarului de aparitii.");
		addWork_Map(analized_file, chunksize, wp);
		
		/* adaugare sarcini in workpool */
		for(String e : files)
			if(!e.equals(analized_file))
				addWork_Map(e, chunksize, wp);
		
		/* initializare si pornire workeri */
		for(int i = 0; i < num_workers; i++)
			workers.add(new My_Worker(wp));
		
		for(My_Worker e : workers)
			e.start();
		
		for(My_Worker e : workers)
			try {
				e.join();
			} catch (InterruptedException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}

		workers.clear();
		
		/*--------------------------------------------------------*/
		/* Etapa de analiza a similaritatii. */
		System.out.println();
		System.out.println("Etapa de analiza a similaritatii");
		/* adaugare sarcini in workpool */
		addWork_Similarity(analized_file, files, wp);
		
		/* initializare si pornire workeri */
		for(int i = 0; i < num_workers; i++)
			workers.add(new My_Worker(wp));
		
		for(My_Worker e : workers)
			e.start();
		
		for(My_Worker e : workers)
			try {
				e.join();
			} catch (InterruptedException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
		
		/*--------------------------------------------------------*/
		/* Scrierea rezultatelor in fisier */
		System.out.println("Frecvente similaritate:");
		System.out.println(My_Worker.my_similarity_map);
		
		/* Folosim un TreeMap pentru a sorta documentele in functie de gradul de similaritate */
		TreeMap<Float, String> resultMap = new TreeMap<Float, String>();
		for(String e: My_Worker.my_similarity_map.keySet())
			resultMap.put(My_Worker.my_similarity_map.get(e), e);
		
		/* Scriem rezultatul */
		try {
			BufferedWriter br = new BufferedWriter(new FileWriter(new File(file_out)));
			
			while(!resultMap.isEmpty())
			{
				Map.Entry<Float, String> e = resultMap.pollLastEntry(); 
				if(e.getKey().floatValue() > accuracy && !e.getValue().equals(analized_file))
					br.write(e.getValue() + " " + ((int)(e.getKey() * 1000))/1000f + "\n");
					
			}
			
			br.close();
		} catch (IOException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		

		
	}
}
