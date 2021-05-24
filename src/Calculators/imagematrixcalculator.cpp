#include "imagematrixcalculator.h"
#include <vector>

using namespace std;

ImageMatrixCalculator::ImageMatrixCalculator(Vector3i step):
    m_step(step)
{

}

Color rgba(double ratio, float alpha)
{
    //we want to normalize ratio so that it fits in to 6 regions
    //where each region is 256 units long
    int normalized = int(ratio * 256 * 6);

    //find the distance to the start of the closest region
    int x = normalized % 256;

    int red = 0, grn = 0, blu = 0;
    switch(normalized / 256)
    {
    case 0: red = 255;      grn = x;        blu = 0;       break;//red
    case 1: red = 255 - x;  grn = 255;      blu = 0;       break;//yellow
    case 2: red = 0;        grn = 255;      blu = x;       break;//green
    case 3: red = 0;        grn = 255 - x;  blu = 255;     break;//cyan
    case 4: red = x;        grn = 0;        blu = 255;     break;//blue
    case 5: red = 255;      grn = 0;        blu = 255 - x; break;//magenta
    }

    return Color(red, grn, blu, alpha);
}

void getCofactor(vector<vector<double>>& mat, vector<vector<double>>& temp,
                 int p, int q, int n)
{
    int i = 0, j = 0;

    // Looping for each element of the matrix
    for (int row = 0; row < n; row++)
    {
        for (int col = 0; col < n; col++)
        {
            //  Copying into temporary matrix only those
            //  element which are not in given row and
            //  column
            if (row != p && col != q)
            {
                temp[i][j++] = mat[row][col];

                // Row is filled, so increase row index and
                // reset col index
                if (j == n - 1)
                {
                    j = 0;
                    i++;
                }
            }
        }
    }
}

double determinantOfMatrix(vector<vector<double>> mat, int n)
{
    double D = 0; // Initialize result

    //  Base case : if matrix contains single element
    if (n == 1)
        return mat[0][0];

    vector<vector<double>> temp(n, vector<double>(n)); // To store cofactors

    int sign = 1; // To store sign multiplier

    // Iterate for each element of first row
    for (int f = 0; f < n; f++)
    {
        // Getting Cofactor of mat[0][f]
        getCofactor(mat, temp, 0, f, n);
        D += sign * mat[0][f]
             * determinantOfMatrix(temp, n - 1);

        // terms are to be added with alternate sign
        sign = -sign;
    }

    return D;
}

const std::deque<ImageData> &ImageMatrixCalculator::Calculate(Program &program, ImageType type, std::function<void(ImageData&)> iterFunc)
{
    auto args = program.GetArgs();

    if(args.size() == 1)
        return matrix1(program, type, iterFunc);
    else if(args.size() == 2)
        return matrix2(program, type, iterFunc);
    else
        return matrix3(program, type, iterFunc);
}

const std::deque<ImageData> &ImageMatrixCalculator::matrix1(Program &program, ImageType type, std::function<void (ImageData &)> iterFunc)
{
    auto args = program.GetArgs();
    Vector3 size{
        (args[0].limits.second-args[0].limits.first)/m_step.x(),
                (args[0].limits.second-args[0].limits.first)/m_step.x(),
                (args[0].limits.second-args[0].limits.first)/m_step.x(),
    };
    Vector3 halfSize{
        size.x()/2.,
                size.x()/2.,
                size.x()/2.,
    };

    double x = args[0].limits.first + halfSize.x();
    while(x < args[0].limits.second)
    {
        vector<pair<Vector3, double>> values{
            {{ x+halfSize.x(), 0, 0 }, 0},
            {{ x-halfSize.x(), 0, 0 }, 0}
        };
        for(int i = 0; i < 2; i++)
            values[i].second = program.Compute(values[i].first);
        x+= size.x();
    }
    return *m_results;
}

const std::deque<ImageData> &ImageMatrixCalculator::matrix2(Program &program, ImageType type, std::function<void (ImageData &)> iterFunc)
{
    auto args = program.GetArgs();
    Vector3 size{
        (args[0].limits.second-args[0].limits.first)/m_step.x(),
                (args[1].limits.second-args[1].limits.first)/m_step.y(),
                (args[1].limits.second-args[1].limits.first)/m_step.y(),
    };
    Vector3 halfSize{
        size.x()/2.,
                size.y()/2.,
                size.y()/2.,
    };

    vector<double> zv(3);
    double x = args[0].limits.first + halfSize.x();
    while(x < args[0].limits.second)
    {
        double y = args[1].limits.first + halfSize.y();
        while(y < args[1].limits.second)
        {
            zv[0] = program.Compute({x, y, 0});
            zv[1] = program.Compute({x+size.x(), y, 0});
            zv[2] = program.Compute({x, y+size.y(), 0});

            int flag = 0;
            for(auto& i: zv)
                if(i >= 0)
                    flag++;

            vector<vector<double>> a{
                {y,          zv[0], 1},
                {y,          zv[1], 1},
                {y+size.y(), zv[2], 1},
            };
            vector<vector<double>> b{
                {x,          zv[0], 1},
                {x+size.x(), zv[1], 1},
                {x,          zv[2], 1},
            };
            vector<vector<double>> c{
                {x,          y,          1},
                {x+size.x(), y,          1},
                {x,          y+size.y(), 1},
            };
            vector<vector<double>> d{
                {x,          y,          zv[0]},
                {x+size.x(), y,          zv[1]},
                {x,          y+size.y(), zv[2]},
            };

            double detA = -determinantOfMatrix(a, 3);
            double detB = -determinantOfMatrix(b, 3);
            double detC = determinantOfMatrix(c, 3);
            double detD = determinantOfMatrix(d, 3);

            detA *= 100;
            detB *= 100;
            detD *= -1;

            double div = sqrt(pow(detA, 2)+pow(detB, 2)+pow(detC, 2)+pow(detD, 2));

            double nA = detA/div;
            double nB = detB/div;
            double nC = detC/div;
            double nD = detD/div;

            double color;
            double grad = 500;
            if(type == ImageType::Cx)
            {
                color = (nA+1)/grad;
            }
            else if(type == ImageType::Cy)
            {
                color = (nB+1)/grad;
            }
            else if(type == ImageType::Cz)
            {
                color = (nC+1)/grad;
            }
            else if(type == ImageType::Cw)
            {
                color = (nD+1)/grad;
            }
            else if(type == ImageType::Ct)
            {
                color = (nD+1)/grad;
            }

            if(flag < 2)
                m_results->push_back(ImageData({x, y, 0}, halfSize, Color(color*grad/2, 0, color*grad/2, 1), zv[0], 2));
            else
                m_results->push_back(ImageData({x, y, 0}, halfSize, Color(color*grad/2, color*grad/2, color*grad/2, 1), zv[0], 2));

            if(iterFunc)
                iterFunc(m_results->back());
            y+= size.y();
        }
        x+= size.x();
    }
    return *m_results;
}

double det4(vector<vector<double>> m)
{
    return m[0][3] * m[1][2] * m[2][1] * m[3][0] - m[0][2] * m[1][3] * m[2][1] * m[3][0] -
            m[0][3] * m[1][1] * m[2][2] * m[3][0] + m[0][1] * m[1][3] * m[2][2] * m[3][0] +
            m[0][2] * m[1][1] * m[2][3] * m[3][0] - m[0][1] * m[1][2] * m[2][3] * m[3][0] -
            m[0][3] * m[1][2] * m[2][0] * m[3][1] + m[0][2] * m[1][3] * m[2][0] * m[3][1] +
            m[0][3] * m[1][0] * m[2][2] * m[3][1] - m[0][0] * m[1][3] * m[2][2] * m[3][1] -
            m[0][2] * m[1][0] * m[2][3] * m[3][1] + m[0][0] * m[1][2] * m[2][3] * m[3][1] +
            m[0][3] * m[1][1] * m[2][0] * m[3][2] - m[0][1] * m[1][3] * m[2][0] * m[3][2] -
            m[0][3] * m[1][0] * m[2][1] * m[3][2] + m[0][0] * m[1][3] * m[2][1] * m[3][2] +
            m[0][1] * m[1][0] * m[2][3] * m[3][2] - m[0][0] * m[1][1] * m[2][3] * m[3][2] -
            m[0][2] * m[1][1] * m[2][0] * m[3][3] + m[0][1] * m[1][2] * m[2][0] * m[3][3] +
            m[0][2] * m[1][0] * m[2][1] * m[3][3] - m[0][0] * m[1][2] * m[2][1] * m[3][3] -
            m[0][1] * m[1][0] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
}

const std::deque<ImageData> &ImageMatrixCalculator::matrix3(Program &program, ImageType type, std::function<void (ImageData &)> iterFunc)
{
    auto args = program.GetArgs();
    Vector3 size{
        (args[0].limits.second-args[0].limits.first)/m_step.x(),
                (args[1].limits.second-args[1].limits.first)/m_step.y(),
                (args[2].limits.second-args[2].limits.first)/m_step.z(),
    };
    Vector3 halfSize{
        size.x()/2.,
                size.y()/2.,
                size.z()/2.,
    };
    vector<double> wv(4);
    double x = args[0].limits.first + halfSize.x();
    while(x < args[0].limits.second)
    {
        double y = args[1].limits.first + halfSize.y();
        while(y < args[1].limits.second)
        {
            double z = args[2].limits.first + halfSize.z();
            while(z < args[2].limits.second)
            {
                wv[0] = program.Compute({x, y, z});
                wv[1] = program.Compute({x+size.x(), y, z});
                wv[2] = program.Compute({x, y+size.y(), z});
                wv[3] = program.Compute({x, y, z+size.z()});

                int flag = 0;
                for(auto& i: wv)
                    if(i >= 0)
                        flag++;

                vector<vector<double>> a{
                    {y,          z,          wv[0], 1},
                    {y,          z,          wv[1], 1},
                    {y+size.y(), z,          wv[2], 1},
                    {y,          z+size.z(), wv[3], 1},
                };
                vector<vector<double>> b{
                    {x,          z,          wv[0], 1},
                    {x+size.x(), z,          wv[1], 1},
                    {x,          z,          wv[2], 1},
                    {x,          z+size.z(), wv[3], 1},
                };
                vector<vector<double>> c{
                    {x,          y,          wv[0], 1},
                    {x+size.x(), y,          wv[1], 1},
                    {x,          y+size.y(), wv[2], 1},
                    {x,          y,          wv[3], 1},
                };
                vector<vector<double>> d{
                    {x,          y,          z,          1},
                    {x+size.x(), y,          z,          1},
                    {x,          y+size.y(), z,          1},
                    {x,          y,          z+size.z(), 1},
                };
                vector<vector<double>> f{
                    {x,          y,          z,          wv[0]},
                    {x+size.x(), y,          z,          wv[1]},
                    {x,          y+size.y(), z,          wv[2]},
                    {x,          y,          z+size.z(), wv[3]},
                };

                double detA = determinantOfMatrix(a, 4);
                double detB = determinantOfMatrix(b, 4);
                double detC = determinantOfMatrix(c, 4);
                double detD = determinantOfMatrix(d, 4);
                double detF = determinantOfMatrix(f, 4);

                double div = sqrt(pow(detA, 2)+pow(detB, 2)+pow(detC, 2)+pow(detD, 2)+pow(detF, 2));

                double nA = detA/div;
                double nB = -detB/div;
                double nC = -detC/div;
                double nD = detD/div;
                double nF = detF/div;

                double color;
                double grad = 500;
                if(type == ImageType::Cx)
                {
                    color = (nA+1)/grad;
                }
                else if(type == ImageType::Cy)
                {
                    color = (nB+1)/grad;
                }
                else if(type == ImageType::Cz)
                {
                    color = (nC+1)/grad;
                }
                else if(type == ImageType::Cw)
                {
                    color = (nD+1)/grad;
                }
                else if(type == ImageType::Ct)
                {
                    color = (nF+1)/grad;
                }

//                if(flag < 3)
                m_results->push_back(ImageData({x, y, z}, halfSize, rgba(color, 0.2), wv[0]));
//                else
//                    m_results->push_back(ImageData({x, y, z}, halfSize, Color(color, 0, color, 0.2), wv[0]));

                if(iterFunc)
                    iterFunc(m_results->back());
                z+= size.z();
            }
            y+= size.y();
        }
        x+= size.x();
    }
    return *m_results;
}