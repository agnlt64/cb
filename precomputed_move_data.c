#include <stdlib.h>
#include <math.h>

#include "precomputed_move_data.h"
#include "square.h"

int centre_manhattan_distance[64];
int orthogonal_distance[64][64];

void init_distances()
{
    for (size_t sqA = 0; sqA < 64; sqA++)
    {
        square_t coordA = idx_to_square(sqA);
        int file_dst_from_center = (int)fmaxf(3 - coordA.file, coordA.file - 4);
        int rank_dst_from_center = (int)fmaxf(3 - coordA.rank, coordA.rank - 4);
        centre_manhattan_distance[sqA] = file_dst_from_center + rank_dst_from_center;

        for (size_t sqB = 0; sqB < 64; sqB++)
        {
            square_t coordB = idx_to_square(sqB);
            int rank_dst = abs(coordA.rank - coordB.rank);
            int file_dst = abs(coordA.file - coordB.file);
            orthogonal_distance[sqA][sqB] = file_dst + rank_dst;
        }
    }
}
