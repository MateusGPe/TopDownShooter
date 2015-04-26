/*
A top down 2d shooter
Copyright (C) 2015  Marien Raat

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "shadow.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace sf;

Vector2f normalize(Vector2f vector) {
    float length = sqrt(vector.x * vector.x
                        + vector.y * vector.y);
    return Vector2f(vector.x / length
                    , vector.y / length);
}

// ShadowHandler methods
void ShadowHandler::genObstaclePoints(const char* filepath
                                      , int tileSize) {
    std::ifstream mapReader(filepath);
    char buf[10];
    mapReader >> buf;
    int width, height;
    mapReader >> width >> height;
    mapReader >> buf;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int tileNum;
            mapReader >> tileNum;
            if (tileNum == 1) {
                obstaclePoints.push_back(
                    Vector2f(x * tileSize
                             , y * tileSize));
                obstaclePoints.push_back(
                    Vector2f((x+1) * tileSize
                             , y * tileSize));
                obstaclePoints.push_back(
                    Vector2f((x+1) * tileSize
                             , (y+1) * tileSize));
                obstaclePoints.push_back(
                    Vector2f(x * tileSize
                             , (y+1) * tileSize));
            }
        }
    }
}

void ShadowHandler::update(Vector2f sightCenter) {
    VertexArray vertices;
    vertices.resize(obstaclePoints.size() * 2);
    vertices.setPrimitiveType(Quads);

    float viewDistance = 1000; // TODO change dynamically
    float pi = 3.14159265359;
    for (int i = 0; i < obstaclePoints.size(); i += 4) {
        float rotation[4];
        int r = 0;
        for (int j = i; j < i + 4; j++) {
            Vector2f rayVector = obstaclePoints[j]
                - sightCenter; 
            
            rotation[r] = std::atan2
                (rayVector.x, rayVector.y) + pi;
            r++;
        }
        float maxDistance = 0;
        int minIndex, maxIndex;
        for (int x = 0; x < 4; x++) {
            for (int y = x + 1; y < 4; y++) {
                float distance = fabs(rotation[x]
                                      - rotation[y]);
                if (distance > pi) {
                    float minRot = rotation[x] < rotation[y]
                                                 ? rotation[x]
                                                 : rotation[y];
                    float maxRot = rotation[x] > rotation[y]
                                                 ? rotation[x]
                                                 : rotation[y];
                    distance = fabs(maxRot - (minRot + 2 * pi));
                }
                if (distance > maxDistance) {
                    maxDistance = distance;
                    minIndex = i + x;
                    maxIndex = i + y;
                }
            }
        }
        Vector2f direction1 = normalize
            (obstaclePoints[minIndex] - sightCenter)
            , direction2 = normalize
            (obstaclePoints[maxIndex] - sightCenter);
        float move = 10;
        Vector2f startPoint1 = obstaclePoints[minIndex]
            + (move * direction1)
            , startPoint2 = obstaclePoints[maxIndex]
                  + (move * direction2);
        
        vertices[2*i+0].position = startPoint1;
        vertices[2*i+1].position = startPoint2;
        vertices[2*i+2].position = direction2
            * viewDistance + startPoint2;
        vertices[2*i+3].position = direction1
            * viewDistance + startPoint1;

        // Draw black rectangle in shape of the original
        // obstacle
        vertices[2*i+4].position = obstaclePoints[i+0] + (move * normalize(obstaclePoints[i+0] - sightCenter));
        vertices[2*i+5].position = obstaclePoints[i+1] + (move * normalize(obstaclePoints[i+1] - sightCenter));
        vertices[2*i+6].position = obstaclePoints[i+2] + (move * normalize(obstaclePoints[i+2] - sightCenter));
        vertices[2*i+7].position = obstaclePoints[i+3] + (move * normalize(obstaclePoints[i+3] - sightCenter));

    }
    for (int i = 0; i < vertices.getVertexCount(); i++) {
        vertices[i].color = Color::Black;
    }
    shadows.setVertices(vertices);
}

void ShadowHandler::draw(RenderWindow* window) {
    window->draw(shadows);
}

// ShadowDrawable methods
void ShadowDrawable::setVertices(VertexArray nVertices) {
    vertices = nVertices;
}

void ShadowDrawable::draw(RenderTarget& target
                         , RenderStates states) const {
    states.transform *= getTransform();
    target.draw(vertices);
}

// Intersection functions
bool onSegment(Vector2f line1, Vector2f line2
               , Vector2f point) {
    return point.x < std::max(line1.x, line2.x)
        && point.x > std::min(line1.x, line2.x)
        && point.y < std::max(line1.y, line2.y)
        && point.y > std::min(line1.y, line2.y);
}

int pointsOrientation(Vector2f point1
                      , Vector2f point2
                      , Vector2f point3) {
// Magic value :). More info:
// http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf
    int value = ((point2.y - point1.y)
                 * (point3.x - point2.x))
        - ((point2.x - point1.x)
           * (point3.y - point2.y));
    if (value == 0)
        return 0; // The points are colinear

    return (value > 0) ? 1 : 2;
}

bool lineSegmentsIntersect(Vector2f line1Point1
                           , Vector2f line1Point2
                           , Vector2f line2Point1
                           , Vector2f line2Point2) {
    int orientation1 = pointsOrientation(line1Point1
                                         , line1Point2
                                         , line2Point1);
    int orientation2 = pointsOrientation(line1Point1
                                         , line1Point2
                                         , line2Point2);
    int orientation3 = pointsOrientation(line2Point1
                                         , line2Point2
                                         , line1Point1);
    int orientation4 = pointsOrientation(line2Point1
                                         , line2Point2
                                         , line1Point2);

    if (orientation1 != orientation2
        && orientation3 != orientation4)
        return true;

    if (orientation1 == 0
        && onSegment(line1Point1, line1Point2
                     , line2Point1))
        return true;

    if (orientation2 == 0
        && onSegment(line1Point1, line1Point2
                     , line2Point2))
        return true;

    if (orientation3 == 0
        && onSegment(line2Point1, line2Point2
                     , line1Point1))
        return true;

    if (orientation4 == 0
        && onSegment(line2Point1, line2Point2
                     , line1Point2))
        return true;

    return false;
}
