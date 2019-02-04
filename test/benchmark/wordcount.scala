import org.apache.spark.storage.StorageLevel._

val textFile = sc.textFile("data/wordcount_data.txt").persist(MEMORY_ONLY)

var start = 0L
for (i <- 0 to 3) {
  if (i == 1) {
    start = System.nanoTime()
  }
  println(i)
  val it_start = System.nanoTime()  
  val counts = textFile.flatMap(line => line.split(" ")).map(word => (word, 1)).reduceByKey(_ + _)
  println(counts.count())
  val it_end = System.nanoTime()  
  println("Elapsed time: " + ((it_end - it_start) / 1.0e6) + "ms")
}
val end = System.nanoTime();
println("Elapsed time: " + ((end - start) / 1.0e6) + "ms")

