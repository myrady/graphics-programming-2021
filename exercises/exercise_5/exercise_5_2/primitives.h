//
// Created by Henrique on 9/17/2019.
//

#ifndef GRAPHICSPROGRAMMINGEXERCISES_PRIMITIVES_H
#define GRAPHICSPROGRAMMINGEXERCISES_PRIMITIVES_H

        //--------water---------

std::vector<float> floorVertices {-150.0f, -5.0f, 150.0f,
                                   150.0f, -5.0f, 150.0f,
                                   150.0f, -5.0f, -150.0f,
                                  -150.0f, -5.0f, -150.0f};
std::vector<unsigned int> floorIndices {0, 1, 2,
                                        0, 2, 3};
std::vector<float> floorColors {.01f, .1f, .2f, 0.033f,
                                .01f, .1f, .2f, 0.033f,
                                .01f, .1f, .2f, 0.033f,
                                .01f, .1f, .2f, 0.033f};


#endif //GRAPHICSPROGRAMMINGEXERCISES_PRIMITIVES_H
