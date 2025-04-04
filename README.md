# Escape Time Fractals

The simplest algorithm for generating visual representations of the Mandelbrot set is the escape-time algorithm. This method repeatedly evaluates a calculation for each coordinate (x, y) within the plotting domain, assigning pixel colors based on the behavior of the resulting orbit sequence.

Essentially, the algorithm tracks the number of iterations required to detect divergence of the orbit {z₀, z₁, z₂, …, zₙ}. Mathematical analysis confirms that if any point zₙ surpasses a predefined boundary region R, the orbit will inevitably diverge toward infinity. Although the exact geometry and minimum dimensions of R depend on the fractal type, the Mandelbrot set conventionally employs a circle centered at the origin with radius 2, as exceeding |z| > 2 ensures divergence.

In practice, the iteration count n at which the sequence exits R determines the coloring value. This approach inherently limits iterations, preventing infinite loops during computation. While mathematical rigor dictates a boundary of radius 2, creative adaptations involving varied shapes—such as ellipses, triangles, or stars—and smaller radii are frequently explored for artistic experimentation.

![alt text](./assets/Fractal.png)

## Installation and Usage


## Credits
Research Group:
- Adrian Batista-Planas
- Ernesto Quintas-Sanchez
- Richard Dawes (Advisor)

This work was partially supported by the Missouri University of Science and Technology’s Kummer Institute for Student Success.