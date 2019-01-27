import org.apache.spark.mllib.clustering.{KMeans, KMeansModel}
import org.apache.spark.mllib.linalg.Vectors
import org.apache.spark.storage.StorageLevel._

// Load and parse the data
val data = sc.textFile("data/cluster_data_kmeans.txt")
val parsedData = data.map(s => Vectors.dense(s.split(' ').map(_.toDouble))).cache()


// Cluster the data into classes using KMeans
var start = 0L
for (i <- 0 to 10) {
  if (i == 1) {
    start = System.nanoTime()
  }
  println(i)
  val model = new KMeans()
  model.setK(5)
  val initialModel = new KMeansModel(
    Array("[0.0, 0.0, 0.0]", "[1.0, 0.0, 0.0]", "[2.0, 0.0, 0.0]", "[3.0, 0.0, 0.0]", "[4.0, 0.0, 0.0]").map(Vectors.parse(_))
  )
  model.setInitialModel(initialModel)
  model.setEpsilon(1.0e-10)
  val it_start = System.nanoTime()
  val res = model.run(parsedData)
  val it_end = System.nanoTime()
  println(res.clusterCenters.deep.mkString("\n"))
  println("Elapsed time: " + ((it_end - it_start) / 1.0e6) + "ms")
}
val end = System.nanoTime()
println("Elapsed time: " + ((end - start) / 1.0e6) + "ms")
