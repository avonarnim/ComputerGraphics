1. To render the spline, I just used the brute force method to calculating intermediary points. I do this within my display function using a call to `calculateSplinePoint`, which performs matrix multiplication to find a point bounded by 4 inputted control points.

To get rid of repetitive code (from iteration cases such as the first 3 elements or the last three elements), I start iterating through points starting on the fourth and have a consistent downshift of control points. Thus, on the first iteration of my spline-rendering for-loop, my first four points are the first four points of the spline (though I assign a dummy point value outside of the loop).

2. To render the ground, I used a plane of size 1000x1000.

3. To render the sky, I used four planes of size 1000x500 so that the viewer seems to see the landscape moreso than the sky. I then used a plane of 1000x1000 to mirror the ground plane and used a different texture than the side plan to give it a greater sense of up versus down.

4. To implement the ride, I used a function called `positionCamera` to update coordinates for the camera. It is called within `doIdle` and subsequently calls `glutPostRedisplay`. `Display` uses the camera coordinates in a call to `gluLookAt`, using the camera coordinates as the eye position so that whenever the coordinates of the camera change, the scene changes appropriately.

Also for the eye position in `gluLookAt`, I added a constant factor multiplied by the normal vector to the camera coordinates so that the viewer is always slightly above the track. This helps give viewers a feeling that they're actually riding the roller coaster as opposed to being inside it.

For the look-at position in gluLookAt, I used the camera position plus the tangent vector. This means that the camera will look in the forward-direction of the spline. For the up vector, I used the normal vector at the given camera position, which I calculate during `positionCamera`.

5. I chose to render a rectangular-prism cross section for the tracks. I did so by creating six quads per sequential pair of points with each quad being constructed as some combination of clockwise-connected points, each composed of a base point ± a constant multiple of the normal vector ± a constant multiple of the binormal vector. These vector values were calculated in the `display` function.

Extras:

1. I rendered a double rail by creating two separate spline renderings each offset by some constant multiple of the binormal vector.
2. I added some OpenGL lighting, placing two points below the track so that the sky is relatively brighter than the ground.
