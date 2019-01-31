import org.apache.spark.graphx.GraphLoader
import org.apache.spark.graphx.lib.PageRank
import org.apache.spark.storage.StorageLevel._

val graph = GraphLoader.edgeListFile(sc, "data/pagerank_data.txt").cache()

var t0 = 0L
for (i <- 0 to 3) {
  if (i == 1) {
    t0 = System.nanoTime()
  }

  val t00 = System.nanoTime()
  val ranks = PageRank.runUntilConvergence(graph, 1.0e-5).vertices
  println(ranks.values.count())
  val t11 = System.nanoTime()

  println("Elapsed time: " + ((t11 - t00) / 1.0e6) + "ms")
  ranks.foreach { t =>
    if (t._1 % 200000 == 1) {
      println(t)
    }
  }
  println(ranks.values.sum())
}
val t1 = System.nanoTime()
println("Elapsed time: " + ((t1 - t0) / 1.0e6) + "ms")
