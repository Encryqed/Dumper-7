#pragma once

namespace MultiLevelCapture
{
	/* Iterates the current GObjects array, pins any newly-seen UClass slots against GC
	 * (via FUObjectItem::RootSet), and accumulates them in an internal set keyed by
	 * object-array index. Safe to call repeatedly across level transitions. */
	void CaptureCurrentClasses();

	/* Resolves GameName/GameVersion via KismetSystemLibrary if not already set, then
	 * calls Generator::InitInternal() followed by all four generator passes. */
	void RunGenerators();

	/* Verifies that at least one F4 capture has been performed, then calls RunGenerators()
	 * with wall-clock timing printed to stderr. */
	void GenerateSDK();
}
