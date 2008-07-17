// OpProfile.h

#include "../../HeeksCAD/interface/HeeksObj.h"
#include "../../HeeksCAD/interface/Box.h"
#include "DropCutter.h"
#include "OpMove3D.h"
#include "HeeksCNCTypes.h"

class COpProfile : public HeeksObj{
private:
	int m_display_list;
	CBox m_box;
	std::string m_title;
	static wxIcon* m_icon;

	void make_tri_list(std::list<GTri> &tri_list, CBox &box);
	void calculate_toolpath(bool call_WasModified = true); // do it

public:
	std::list<HeeksObj*> m_objects; // lines and arcs, for now
	std::list<CMove3D> m_toolpath;
	int m_station_number;
	Cutter m_cutter;
	int m_offset_left; // 1 = left, -1 = right, 0 = no offset, just profile along shape
	int m_lead_on_type; // 0 = just go straight to start, 1 = use m_lead_on_radius;
	wxString m_toolpath_failed;

	bool m_attach_to_solids;
	std::list<HeeksObj*> m_solids;
	double m_deflection;// for triangulation
	double m_little_step_length; // along the zig
	double m_low_plane;
	bool m_out_of_date;
	bool m_only_calculate_if_user_says;

	COpProfile();
	COpProfile(const COpProfile &p):m_display_list(0), m_cutter(1.0, 0.0){operator=(p);}
	~COpProfile();

	const COpProfile &operator=(const COpProfile &p);

	int GetType()const{return OpProfileType;}
	const char* GetShortString(void)const{return m_title.c_str();}
	const char* GetTypeString(void)const{return "Profile Operation";}
	bool CanEditString(void)const{return true;}
	void OnEditString(const char* str){m_title.assign(str);}
	void glCommands(bool select, bool marked, bool no_color);
	void GetBox(CBox &box);
	void KillGLLists(void);
	wxIcon* GetIcon();
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);

	void CalculateOrMarkOutOfDate(bool call_WasModified = true);
	void ForceCalculateToolpath();
};