#include <ndebugoverlay.h>

void DebugDrawLine(const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration)
{
	NDebugOverlay::Line(vecAbsStart + Vector(0, 0, 0.1), vecAbsEnd + Vector(0, 0, 0.1), r, g, b, test, duration);
}