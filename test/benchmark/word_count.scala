val textFile = sc.textFile("data/bible+shakes.nopunc")
val lines = textFile.flatMap(line => line.split(" ")).cache()

var start = 0L
for (i <- 0 to 3) {
  if (i == 1) {
    start = System.nanoTime()
  }
  println(i)
  val it_start = System.nanoTime()  
  val counts = lines.map(word => (word, 1)).reduceByKey(_ + _)
  println(counts.count())
  val it_end = System.nanoTime()  
  println("Elapsed time: " + ((it_end - it_start) / 1.0e6) + "ms")
}
val end = System.nanoTime();
println("Elapsed time: " + ((end - start) / 1.0e6) + "ms")

