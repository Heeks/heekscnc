
// HeeksCNCTypes.h

#pragma once

// NOTE: If adding to this enumeration, please also update the HeeksCNCType() routine.

enum{
	ProgramType = 10001,
	NCCodeBlockType,
	NCCodeType,
	OperationsType,
	ProfileType,
	PocketType,
	ZigZagType,
	AdaptiveType,
	DrillingType,
	ToolType,
	ToolsType,
	TurnRoughType,
	FixtureType,
	FixturesType,
	SpeedReferenceType,
	SpeedReferencesType,
	CuttingRateType,
	PositioningType,
	TrsfNCCodeType,
	ProbeCentreType,
	ProbeEdgeType,
	ContourType,
	ChamferType,
	InlayType,
	ProbeGridType,
	TagsType,
	TagType,
	ScriptOpType,
	AttachOpType,
	UnattachOpType,
	WaterlineType,
	RaftType,
	TappingType,
	HeeksCNCMaximumType
};
