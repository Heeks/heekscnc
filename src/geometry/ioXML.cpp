// written by g.j.hawkesford 2006 for Camtek Gmbh

// simple XML output classes to implement sufficient for p4c/peps purposes
// 
#include "geometry.h"
#ifdef WIN32
#include "windows.h"
#endif
#include <iomanip>

namespace geoff_geometry {

	inputXML::inputXML(const std::wstring& fname) : ip(NULL){		
		// for some reason wifstream doesn't support wide string filename????
		ip = new wifstream(_wfopen(fname.data(),L"r"));
		if(ip == NULL || ip->is_open() == false){
			wchar_t l[1024];
			wcscpy(l, L"Cannot open input XML file - ");
			wcscat(l, (wchar_t *)fname.data());
			FAILURE(l);
		}
		scale = 1.0;
	}

	inputXML::inputXML(const wchar_t* fname) : ip(NULL){
		// for some reason wifstream doesn't support wide string filename????
		ip = new wifstream(_wfopen(fname,L"r"));

		if(ip == NULL || ip->is_open() == false){
			wchar_t l[1024];
			wcscpy(l, L"Cannot open input XML file - ");
			wcscat(l, (wchar_t *)fname);
			FAILURE(l);
		}
		scale = 1.0;
	};

	inputXML::~inputXML(){
		if(ip->is_open() == true) ip->close();
		if(ip != NULL) delete ip;
	};

	bool inputXML::startElement(const wchar_t* name) {
		// first move to after next "<"
		wchar_t ch;
		while((ch = ip->get()) != '<'){
			if(ch == char_traits<wchar_t>::eof()) return false;
		}

		return findString(std::wstring(name));
	};

	bool inputXML::startElement(const std::wstring& name) {
		// first move to after next "<"
		wchar_t ch;
		while((ch = ip->get()) != '<'){
			if(ch == char_traits<wchar_t>::eof()) return false;
		}
		return findString(name);
	};

	bool inputXML::endElement(){
		// only look to next /..>
		wchar_t ch;
		while((ch = ip->get()) != '/'){
			if(ch == char_traits<wchar_t>::eof()) return false;
		}
		while((ch = ip->get()) != '>'){
			if(ch == char_traits<wchar_t>::eof()) return false;
		}
		return true;
	}

	bool inputXML::findString(const std::wstring& name){
		// compares name to the next string in the input stream
		// if not matched, then returns false and the stream pointer is reset
		int ipos = ip->tellg();
		wchar_t ch;
		while((ch = ip->get()) == '\"');
		while(ch == ' ') ch = ip->get();

		unsigned int i = 0;
		while(ch != '\n') {
			if(ch == char_traits<wchar_t>::eof()) return false;
			if(ch == name.at(i)) {
				if(++i >= name.size()) return true;
			}
			else
				if(i > 0) break;
			ch = ip->get();
		}
		ip->seekg(ipos - 1, ios::beg);
		return false;	
	};

	bool inputXML::Attribute(const wchar_t* name, std::wstring& data) {
		std::wstring n(name);
		n += L"=\"";
		data.clear();
		if(findString(n)) {
			// copy up to closing "
			wchar_t ch;
			while((ch = ip->get()) != '\"') data += ch;
			return true;
		}
		return false;
	}

		
	bool inputXML::Attribute(const wchar_t* name, bool& data) {
		std::wstring str;
		if(Attribute(name, str)) {
			data = str.compare(L"TRUE")?false : true;
			return true;
		}
		return false;
	}


	bool inputXML::Attribute(const wchar_t* name, int& data) {
		std::wstring str;
		if(Attribute(name, str)) {
			data = _wtoi(str.data());
			return true;
		}
		return false;
	}

	bool inputXML::Attribute(const wchar_t* name, double& data, bool scaledata) {
		std::wstring str;
		if(Attribute(name, str)) {
			data = _wtof(str.data());
			if(scaledata) data *= scale;
			return true;
		}
		return false;
	}

#if 0
	bool inputXML::Attribute(wchar_t* name, wchar_t* data) {
		std::wstring str;
		if(Attribute(name, str)) {
			data = (wchar_t*)str.data();
			return true;
		}
		return true;
	}
#endif

	void inputXML::Units() {
		int units;
		double toler;
		startElement(L"SYSTEM");
		std::wstring unit;
		Attribute(L"UNITS", unit);
		Attribute(L"TOLERANCE", toler);
		if(unit.compare(L"mm") == 0) units = MM;
		else if(unit.compare(L"inches") == 0) units = INCHES;
		else if(unit.compare(L"metres") == 0) units = METRES;
		if(units != UNITS) {
			if(units == MM) {
				if(UNITS == METRES) scale = .001;
				else if(UNITS == INCHES) scale = 1. / 25.4;
			}
			else if(units == METRES) {
				if(UNITS == MM) scale = 1000.;
				else if(UNITS == INCHES) scale = 1000. / 25.4;
			}
			else if(units == INCHES) {
				if(UNITS == MM) scale = 25.4;
				else if(UNITS == METRES) scale = 25.4 / 1000.;
			}
			TOLERANCE = toler * scale;
		}
		else {
			scale = 1.0;
			TOLERANCE = toler;
		}

		endElement();
	}
	bool inputXML::Read(const Point& p, std::wstring& name) {
		bool good = false;
		if(startElement(L"POINT") == true) {
			Attribute(L"name", name);
			good = Attribute(L"x", p.x);
			good &= Attribute(L"y", p.y);
		}
		endElement();
		return p.ok = good;
	}

	bool inputXML::Read(const spVertex& spv, std::wstring& name) {
		// read XML format spVertex
		bool good = false;
		if(startElement(L"SPVERTEX") == true) {
			Attribute(L"name", name);
			std::wstring id;
			good = Attribute(L"ID", id);
			spv.spanid = 0;
//			good = Attribute(L"ID", spv.spanid);
			std::wstring type;
			good &= Attribute(L"type", type);
			if(type.compare(L"LINEAR") == 0) spv.type = LINEAR;
			else if(type.compare(L"CW") == 0 ) spv.type = CW;
			else spv.type = ACW;


			std::wstring pname;
			good &= Read(spv.p, pname);
			if(spv.type != LINEAR) {
				good &= Read(spv.pc, pname);
			}
			endElement();
		}
		return good;
	}

	bool inputXML::Read(const Kurve& k, std::wstring& name) {
		// debug - read next kurve in XML format
		int nVertices;
		bool good = false;
		k.~Kurve();
		good = startElement(L"KURVE");
		Attribute(L"name", name);
		good &= Attribute(L"nVertices", nVertices);
		if(good) {
			if(startElement(L"MATRIX")) {
				bool b;
				if(Attribute(L"UnitMatrix", b) == false) {
					wchar_t o[8];
					for(int i = 0; i < 12; i++) {
						swprintf(o, L"M%d", i);
						Attribute(o, k.e[i], false);
					}
					endElement();
					k.IsUnit();
					k.IsMirrored();
				}
			}


			spVertex spv;
			std::wstring spname;
			for(int i = 0; i < nVertices; i++) {
				if(Read(spv, spname) == false) return false;
				k.Add(spv);
			}
			endElement();
		}
		return good;
	}

	outXML::outXML(const wchar_t* fname, bool scaleToMM):op(NULL){
		op = new wofstream(_wfopen(fname,L"w+"));

		if(op == NULL || op->is_open() == false){
			wchar_t l[1024];
			wcscpy(l, L"Cannot open output XML file - ");
			wcscat(l, (wchar_t *)fname);
			FAILURE(l);
		}
		lastClosed = true;
		*op << setprecision(14);
		scale = (scaleToMM == true)?((UNITS == METRES)?1000. : 1.) :1.;
	}

	outXML::outXML(const std::wstring& fname, bool scaleToMM):op(NULL){
		op = new wofstream(_wfopen(fname.data(),L"w+"));
		
		if(op == NULL || op->is_open() == false){
			wchar_t l[1024];
			wcscpy(l, L"Cannot open output XML file - ");
			wcscat(l, (wchar_t *)fname.data());
			FAILURE(l);
		}
		lastClosed = true;
		*op << setprecision(14);
		scale = (scaleToMM == true)?((UNITS == METRES)?1000. : 1.) :1.;
	}

	outXML::~outXML() {
		if(op->is_open() == true) op->close();
		if(op != NULL) delete op;
		if(element.size() != 0) FAILURE(L"Unmatched startElement/endElement - data still in list");
	};

	void outXML::startElement(const wchar_t* ele){
		std::wstring elem(ele);
		startElement(elem);
	}

	void outXML::startElement(const std::wstring& ele) {
		element.push_back(ele);
		if(lastClosed == false) *op << ">\n";
		*op << "<" << ele.data();
		lastClosed = false;
	}


	void outXML::endElement() {
		if(element.size() == 0) FAILURE(L"Element not started or unmatched startElement/endElement");
		std::list<std::wstring>::iterator ie;
		ie = element.end();
		ie--;
		std::wstring e = *ie;

		element.pop_back();
		if(lastClosed)
			*op << L"</" << e.data() << L">\n";
		else
			*op << L"/>\n";
		lastClosed = true;
	}

	void outXML::Attribute(const std::wstring& att, int data){
		*op << L" " << att.data() << L"=\"" << data << '\"';
	}
	void outXML::Attribute(const wchar_t* att, int data){
		std::wstring attr(att);
		Attribute(attr, data);
	}
	void outXML::Attribute(const std::wstring& att, bool data){
		*op << L" " << att.data() << L"=\"" << ((data)?L"TRUE" : L"FALSE") << '\"';
	}
	void outXML::Attribute(const wchar_t* att, bool data){
		std::wstring attr(att);
		Attribute(attr, data);
	}

	void outXML::Attribute(const std::wstring& att, double data, bool scaleData){
		if(scaleData == true) data *= scale;
		*op << L" " << att.data() << L"=\"" << data << '\"';
	}
	void outXML::Attribute(const wchar_t* att, double data, bool scaleData){
		std::wstring attr(att);
		Attribute(attr, data, scaleData);
	}
	void outXML::Attribute(const std::wstring& att, const std::wstring& data){
	//	if(data.size() == 0) return;
		*op << L" " << att.data() << L"=\"" << data.data() << '\"';
	}
	void outXML::Attribute(const wchar_t* att, const wchar_t* data){
		std::wstring attr(att);
		std::wstring datas(data);
		Attribute(attr, datas);
	}
	void outXML::Attribute(const std::wstring& att, const wchar_t* data){
		std::wstring datas(data);
		Attribute(att, datas);
	}
	void outXML::Attribute(const wchar_t* att, std::wstring data){
		std::wstring attr(att);
		Attribute(attr, data);
	}
	void outXML::Attribute(const wchar_t* att, const void* data) {
		std::wstring attr(att);
		wchar_t d[64];
		_ui64tow((unsigned __int64) data, d, 16);
		Attribute(attr, d);
	}

		

	void outXML::Attribute(const Point& p) {
		Attribute(L"x", p.x);
		Attribute(L"y", p.y);
	}
	void outXML::Attribute(const Point3d& p) {
		Attribute(L"x", p.x);
		Attribute(L"y", p.y);
		Attribute(L"z", p.z);
	}
	void outXML::Units(void) {
		startElement(L"SYSTEM");
		Attribute(L"UNITS", (UNITS == MM)?L"mm" : L"inches");
		Attribute(L"TOLERANCE", TOLERANCE);
		endElement();
	}


	void outXML::Write(const spVertex& sp, wchar_t* name, double scale) {
		startElement(L"SPVERTEX");
		Attribute(L"name", name);

		std::wstring spanid;
		switch(sp.spanid) {
			case UNMARKED:
				spanid = L"UNMARKED";
				break;
			case ROLL_AROUND:
				spanid = L"ROLL_AROUND RADIUS";
				break;
			case INTERSECTION:
				spanid = L"INTERSECTION";
				break;

			default:
		//		spanid = L"UNSET";
				wchar_t nID[64];
				swprintf(nID, L"%d", sp.spanid);
				spanid = nID;
				break;
		}
		Attribute(L"ID", spanid);

		if(sp.type == LINEAR) {
			Attribute(L"type", L"LINEAR");
			Write(sp.p, L"", scale);
		}
		else {
			Attribute(L"type", ((sp.type) == CW)?L"CW" : L"ACW");
			Write(sp.p, L"", scale);
			Write(sp.pc, L"centre", scale);
		}		endElement();
	}

	void	outXML::Write(const Matrix& m, wchar_t* name) {
		// debug - output matrix
			startElement(name);
			Attribute(L"UnitMatrix", m.IsUnit()?true : false);
			if(m.m_unit == false) {
				Attribute(L"Mirrored", m.IsMirrored()?true : false);
				Attribute(L"cx - e[0]", m.e[0], false);
				Attribute(L"e[4]", m.e[4], false);
				Attribute(L"e[8]", m.e[8], false);
				Attribute(L"cy - e[1]", m.e[1], false);
				Attribute(L"e[5]", m.e[5], false);
				Attribute(L"e[9]", m.e[9], false);
				Attribute(L"cz - e[2]", m.e[2], false);
				Attribute(L"e[6]", m.e[6], false);
				Attribute(L"e[10]", m.e[10], false);
				Attribute(L"Tr - e[3]", m.e[3], false);
				Attribute(L"e[7]", m.e[7], false);
				Attribute(L"e[11]", m.e[11], false);
			}
			endElement();
	}

	void	outXML::Write(const Kurve& k, wchar_t* name, double scale) {
		// debug - output XML format
		// untransformed
		startElement(L"KURVE");
		Attribute(L"name", name);
		Attribute(L"nVertices", k.nSpans()+1);
		Write((Matrix)k, L"MATRIX");
		for(int i = 0; i <= k.nSpans(); i++) {
			spVertex sp;
			k.Get(i, sp);
			Write(sp, L"", scale);
		}
		endElement();
	}

	void outXML::Comment(const wchar_t* comment) {
		if(lastClosed == false) *op << L">\n";
		lastClosed = true;
		*op << L"<!--" << comment << L"-->\n";
	}
		
	void outXML::Body(const wchar_t* bodytext){
		if(lastClosed == false) *op << L">\n";
		lastClosed = true;
		*op << bodytext;
	}

}