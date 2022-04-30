#ifndef QNOISE_H
#define QNOISE_H

/*
 v1.0 - 05.03.2018

 OpenSimplex Noise in c++
 by Jonas Hilk


 Original Java - implementation by Kurt Spencer
 https://gist.github.com/KdotJPG/b1270127455a94ac5d19

 v1.0  currently limited to 2D and 3D noise
 v1.1 added 4d noise for completeness

 Provide a seed in the constructor
 noise() - will return a double value between -1 and 1
*/

#include <QImage>

#include <cstdint>
#include <vector>

class QNoise
{
public:
    QNoise(int64_t seed = 0);
    QNoise(const std::vector<short> &permutation);

    void setSeed(int64_t seed);

    //Return value between -1 and 1
    double noise(double x, double y);
    double noise(double x, double y, double z);
    double noise(double x, double y, double z, double w);

    // This method was added by Daniel Koitzsch on 2020-01-15
    static QImage create_noise_image(int width, int height, double scale_factor = 5);

private:
    double extrapolate(int xsb, int ysb, double dx, double dy);
    double extrapolate(int xsb, int ysb, int zsb, double dx, double dy, double dz);
    double extrapolate(int xsb, int ysb, int zsb, int wsb, double dx, double dy, double dz, double dw);

private:

    std::vector<short> m_permutation;
    std::vector<short> m_permGradIndex3D;
};

#endif // QNOISE_H
