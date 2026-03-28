#include "Utilities.h"
#include "AEEngine.h"

bool IsMouseOverRect(float rectX, float rectY, float rectWidth, float rectHeight) {

		int mouseX, mouseY;
		AEInputGetCursorPosition(&mouseX, &mouseY);
		float worldX = static_cast<float>(mouseX) - 800.0f; // Convert to world coordinates
		float worldY = 450.0f - static_cast<float>(mouseY); // Invert Y axis and convert
		return worldX >= rectX - rectWidth * 0.5f &&
			   worldX <= rectX + rectWidth * 0.5f &&
			   worldY >= rectY - rectHeight * 0.5f &&
			   worldY <= rectY + rectHeight * 0.5f;
	
}

bool ClickedOnRect(float rectX, float rectY, float rectWidth, float rectHeight) {
	if (IsMouseOverRect(rectX, rectY, rectWidth, rectHeight) && AEInputCheckTriggered(AEVK_LBUTTON)) {
		return true;
	}
	return false;
}

// Helper to format seconds into MM:SS or just seconds
void FormatTime(float seconds, char* buffer, int bufferSize)
{
	if (seconds >= 60.0f)
	{
		int minutes = (int)(seconds / 60.0f);
		int secs = (int)(seconds) % 60;
		sprintf_s(buffer, bufferSize, "%d:%02d", minutes, secs);
	}
	else
	{
		sprintf_s(buffer, bufferSize, "%.0f sec", seconds);
	}
}
