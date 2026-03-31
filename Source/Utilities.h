#pragma once
#ifndef UTILITIES_H
#define UTILITIES_H

bool IsMouseOverRect(float rectX, float rectY, float rectWidth, float rectHeight);

bool ClickedOnRect(float rectX, float rectY, float rectWidth, float rectHeight);

bool isMouseOver4Corners(float rectX1, float rectY1, float rectX2, float rectY2);

void FormatTime(float seconds, char* buffer, int bufferSize);

#endif // UTILITIES_H