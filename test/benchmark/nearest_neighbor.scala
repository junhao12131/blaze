val textFile = spark.read.textFile("data/pagerank_data_small.txt")

var x = (0, 0)

implicit val sortPoints = new Ordering[(Int, Int)] {
  override def compare(a: (Int, Int), b: (Int, Int)) = {
      val diffA1 = a._1 - x._1
      val diffA2 = a._2 - x._2
      val diffB1 = b._1 - x._1
      val diffB2 = b._2 - x._2
      if (diffA1 * diffA1 + diffA2 * diffA2 < diffB1 * diffB1 + diffB2 * diffB2) {
        1
      } else {
        -1
      }
    }
  }

var t0 = 0L
val points = textFile.map(line => line.split(" ")).map{case Array(a, b)=>(a.toInt, b.toInt)}

for (i <- 0 to 10) {
  if (i == 0) {
    t0 = System.nanoTime()
  }
  
  x = (i * i, i * i)
  val t00 = System.nanoTime()
  val res = points.rdd.top(5)
  val t01 = System.nanoTime()
  println(res(0), res(1))
  println("Elapsed time: " + ((t01 - t00) / 1.0e6) + "ms")
}
