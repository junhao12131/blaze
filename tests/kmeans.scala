import org.apache.spark.mllib.clustering.{KMeans, KMeansModel}
import org.apache.spark.mllib.linalg.Vectors

// Load and parse the data
val data = sc.textFile("tests/kmeans_data.txt")
val parsedData = data.map(s => Vectors.dense(s.split(' ').map(_.toDouble))).cache()

val initialModel = new KMeansModel(
   Array("[0.0, 0.0, 0.0]", "[1.0, 0.0, 0.0]", "[2.0, 0.0, 0.0]", "[3.0, 0.0, 0.0]", "[4.0, 0.0, 0.0]").map(Vectors.parse(_))
)

// Cluster the data into classes using KMeans
var t0 = 0L
for (i <- 0 to 10) {
  if (i == 1) {
    t0 = System.nanoTime()
  }
  println(i)
  val model = new KMeans()
  model.setK(5)
  model.setInitialModel(initialModel)
  model.setEpsilon(1.0e-10)
  val res = model.run(parsedData)
  println(res.clusterCenters.deep.mkString("\n"))
}
val t1 = System.nanoTime()
println("Elapsed time: " + ((t1 - t0) / 1.0e6) + "ms")
