// NCCode.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// This is an object that can go in the tree view as a child of a program.
// It contains lists of NC Code blocks, each one is a line of an NC file, but also contains drawing items
// The text of this is shown in the "output" window.

#pragma once

#include "../../interface/HeeksObj.h"
#include "../../interface/HeeksColor.h"
#include "HeeksCNCTypes.h"
#include "CTool.h"

#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <list>

enum ColorEnum{
	ColorDefaultType,
	ColorBlockType,
	ColorMiscType,
	ColorProgramType,
	ColorToolType,
	ColorCommentType,
	ColorVariableType,
	ColorPrepType,
	ColorAxisType,
	ColorRapidType,
	ColorFeedType,
	MaxColorTypes
};

class ColouredText
{
public:
	wxString m_str;
	ColorEnum m_color_type;
	ColouredText():m_color_type(ColorDefaultType){}

	void WriteXML(TiXmlNode *root);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class PathObject{
public:
	typedef enum {
		eLine = 0,
		eArc
	} eType_t;

public:
	static double m_current_x[3];
	static double m_prev_x[3];
	double m_x[3];
	int m_tool_number;
	PathObject(){m_x[0] = m_x[1] = m_x[2] = 0.0;}
	virtual int GetType() = 0; // 0 - line, 1 - arc
	virtual void GetBox(CBox &box,const PathObject* prev_po){box.Insert(m_x);}

	void WriteBaseXML(TiXmlElement *element);

	virtual void WriteXML(TiXmlNode *root) = 0;
	virtual void ReadFromXMLElement(TiXmlElement* pElem) = 0;
	virtual void glVertices(const PathObject* prev_po){}
	virtual PathObject *MakeACopy(void)const = 0;
};

class PathLine : public PathObject{
public:
	int GetType(){return int(PathObject::eLine);}
	void WriteXML(TiXmlNode *root);
	void ReadFromXMLElement(TiXmlElement* pElem);
	void glVertices(const PathObject* prev_po);
	PathObject *MakeACopy(void)const{return new PathLine(*this);}

	std::list<gp_Pnt> Interpolate( const gp_Pnt & start_point,
					const gp_Pnt & end_point,
					const double feed_rate,
					const double spindle_rpm,
					const unsigned int number_of_cutting_edges) const;

};

class PathArc : public PathObject{
public:
	double m_c[3]; // defined relative to previous point ( span start point )
	double m_radius;
	int m_dir; // 1 - anti-clockwise, -1 - clockwise
	PathArc(){m_c[0] = m_c[1] = m_c[2] = 0.0; m_dir = 1;}
	int GetType(){return int(PathObject::eArc);}
	void WriteXML(TiXmlNode *root);
	void ReadFromXMLElement(TiXmlElement* pElem);
	void glVertices(const PathObject* prev_po);
	std::list<gp_Pnt> Interpolate( const PathObject *prev_po, const unsigned int number_of_points ) const;

	PathObject *MakeACopy(void)const{return new PathArc(*this);}

	void GetBox(CBox &box,const PathObject* prev_po);
	bool IsIncluded(gp_Pnt pnt,const PathObject* prev_po);

	std::list<gp_Pnt> Interpolate( const PathObject *previous_object,
					const double feed_rate,
					const double spindle_rpm,
					const unsigned int number_of_cutting_edges) const;
	void SetFromRadius();
};

class ColouredPath
{
public:
	ColorEnum m_color_type;
	std::list< PathObject* > m_points;
	ColouredPath():m_color_type(ColorRapidType){}
	ColouredPath(const ColouredPath& c);
	~ColouredPath(){Clear();}

	const ColouredPath &operator=(const ColouredPath& c);

	void Clear();
	void glCommands();
	void GetBox(CBox &box);
	void WriteXML(TiXmlNode *root);
	void ReadFromXMLElement(TiXmlElement* pElem);
};

class CNCCodeBlock:public HeeksObj
{
	bool m_formatted;
public:
	std::list<ColouredText> m_text;
	std::list<ColouredPath> m_line_strips;
	long m_from_pos, m_to_pos; // position of block in text ctrl
	static double multiplier;

	CNCCodeBlock():m_from_pos(-1), m_to_pos(-1), m_formatted(false) {}

	void WriteNCCode(wxTextFile &f, double ox, double oy);

	// HeeksObj's virtual functions
	int GetType()const{return NCCodeBlockType;}
	HeeksObj *MakeACopy(void)const;
	void glCommands(bool select, bool marked, bool no_color);
	void GetBox(CBox &box);
	void WriteXML(TiXmlNode *root);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void AppendText(wxString& str);
	void FormatText(wxTextCtrl *textCtrl, bool highlighted, bool force_format);
};

class CNCCode:public HeeksObj
{
public:
	static long pos; // used for setting the CNCCodeBlock objects' m_from_pos and m_to_pos
private:
	static std::map<std::string,ColorEnum> m_colors_s_i;
	static std::map<ColorEnum,std::string> m_colors_i_s;
	static std::vector<HeeksColor> m_colors;
	CNCCodeBlock* m_highlighted_block;

public:
	static void ClearColors(void);
	static void AddColor(const char* name, const HeeksColor& col);
	static ColorEnum GetColor(const char* name, ColorEnum def=ColorDefaultType);
	static const char* GetColor(ColorEnum i, const char* def="default");
	static int ColorCount(void) { return m_colors.size(); }
	static HeeksColor& Color(ColorEnum i) { return m_colors[i]; }

	std::list<CNCCodeBlock*> m_blocks;
	int m_gl_list;
	CBox m_box;
	bool m_user_edited; // set, if the user has edited the nc code
	static PathObject* prev_po;
	static int s_arc_interpolation_count;	// How many lines to represent an arc for the glCommands() method?

	CNCCode();
	CNCCode(const CNCCode &p):m_gl_list(0), m_highlighted_block(NULL){operator=(p);}
	virtual ~CNCCode();

	const CNCCode &operator=(const CNCCode &p);
	void Clear();

	// HeeksObj's virtual functions
	int GetType()const{return NCCodeType;}
	const wxChar* GetTypeString(void)const{return _T("NC Code");}
	void glCommands(bool select, bool marked, bool no_color);
	void GetBox(CBox &box);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	bool OneOfAKind(){return true;}
	void SetClickMarkPoint(MarkedObject* marked_object, const double* ray_start, const double* ray_direction);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	static void ReadColorsFromConfig();
	static void WriteColorsToConfig();
	static void GetOptions(std::list<Property *> *list);

	void DestroyGLLists(void); // not void KillGLLists(void), because I don't want the display list recreated on the Redraw button
	void SetTextCtrl(wxTextCtrl *textCtrl);
	void FormatBlocks(wxTextCtrl *textCtrl, int i0, int i1);
	void HighlightBlock(long pos);
	void SetHighlightedBlock(CNCCodeBlock* block);

	std::list< std::pair<PathObject *, CTool *> > GetPaths() const;
};
