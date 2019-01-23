import org.apache.spark.mllib.linalg.{DenseVector, Vectors, DenseMatrix}
import org.apache.spark.mllib.clustering.{GaussianMixture, GaussianMixtureModel}
import org.apache.spark.mllib.stat.distribution.MultivariateGaussian

// Load and parse the data
val data = sc.textFile("data/kmeans_data_small.txt")
// val data = sc.textFile("data/kmeans_data_small.txt")
val parsedData = data.map(s => Vectors.dense(s.split(' ').map(_.toDouble))).cache()

val sigma = DenseMatrix.eye(3)

// Cluster the data into classes using KMeans
var start = 0L
for (i <- 0 to 3) {
  if (i == 1) {
    start = System.nanoTime()
  }
  println(i)
  val model = new GaussianMixture()
  model.setK(5)
  val initialModel = new GaussianMixtureModel(
    Array(1.0, 1.0, 1.0, 1.0, 1.0), 
    Array(
      new MultivariateGaussian(new DenseVector(Array(0.0, 0.0, 0.0)), sigma),
      new MultivariateGaussian(new DenseVector(Array(1.0, 0.0, 0.0)), sigma),
      new MultivariateGaussian(new DenseVector(Array(2.0, 0.0, 0.0)), sigma),
      new MultivariateGaussian(new DenseVector(Array(3.0, 0.0, 0.0)), sigma),
      new MultivariateGaussian(new DenseVector(Array(4.0, 0.0, 0.0)), sigma)))
  model.setInitialModel(initialModel)
  model.setConvergenceTol(1.0e-10)
  println(model.getMaxIterations)
  val it_start = System.nanoTime()
  val res = model.run(parsedData)
  val it_end = System.nanoTime()
  res.gaussians.map(g => println(g.mu))
  println("Elapsed time: " + ((it_end - it_start) / 1.0e6) + "ms")
}
val end = System.nanoTime()
println("Elapsed time: " + ((end - start) / 1.0e6) + "ms")
