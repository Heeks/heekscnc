// written by g.j.hawkesford 2006 for Camtek Gmbh

// simple XML output classes to implement sufficient for p4c/peps purposes
// 
#pragma once

	class inputXML {
	private:
		wifstream* ip;
		double scale;

	public:
		inputXML(std::wstring& fname);
		inputXML(wchar_t* fname);
		~inputXML();
		bool startElement(wchar_t* name);
		bool startElement(std::wstring& name);
		bool endElement();
		bool findString(std::wstring& name);
		bool Attribute(wchar_t* name, int& data);
		bool Attribute(wchar_t* name, bool& data);
		bool Attribute(wchar_t* name, wchar_t* data);
		bool Attribute(wchar_t* name, std::wstring& data);
		bool Attribute(wchar_t* name, double& data, bool scaledata = true);
		bool copyString(wchar_t* to, std::wstring& data);
		bool copyString(std::wstring& to, std::wstring& data);
		void Units();
		void setScale(double sca){ scale = sca;};
		bool Read(Point& p, std::wstring& name);
		bool Read(Kurve& k, std::wstring& name);
		bool Read(spVertex& spv, std::wstring& name);


	};


	class outXML {
	private:
		bool lastClosed;
		double scale;			// the scale for scalars
		std::list<std::wstring> element;
		wofstream* op;

	public:
		outXML(wchar_t* fname, bool m_scaleToMM = false);
		outXML(std::wstring& fname, bool m_scaleToMM = false);
		~outXML();
		void Write(Point& p, wchar_t* name, double scale = 1.0);
		void Write(Point3d& p, wchar_t* name);
		void Write(Vector2d& v, wchar_t* name);
		void Write(Vector3d& v, wchar_t* name);
		void Write(CLine& cl, wchar_t* name);
		void Write(Circle& c, wchar_t* name);
		void Write(Matrix& m, wchar_t* name);
		void Write(Kurve& k, wchar_t* name, double scale = 1.0);
		void Write(spVertex& sp, wchar_t* name = L"", double scale = 1.0);
		void startElement(wchar_t* ele);
//		void startElement(wchar_t* ele);
		void startElement(std::wstring& ele);
		void endElement();
		void Attribute(std::wstring& att, int data);
		void Attribute(wchar_t* att, int data);
		void Attribute(std::wstring& att, bool data);
		void Attribute(wchar_t* att, bool data);
		void Attribute(std::wstring& att, double data, bool scaleData = true);
		void Attribute(wchar_t* att, double data, bool scaleData = true);
		void Attribute(std::wstring& att, std::wstring& data);
//		void Attribute(wchar_t* att, char* data);
		void Attribute(wchar_t* att, wchar_t* data);
		void Attribute(std::wstring& att, wchar_t* data);
		void Attribute(wchar_t* att, std::wstring data);
		void Attribute(wchar_t* att, void* data);
		void Attribute(Point& p);
		void Attribute(Point3d& p);
		void Units(void);
		void Comment(wchar_t* comment);
		void Body(wchar_t* bodytext);
	};

#if 0
	// some XML istream methods
	Kurve	iXML(istream& ip, char* name, double scale);			// input Kurve in XML format
	bool iXMLUnits(istream& ip, int& units, double& tolerance);
	bool FindXMLAttribute(istream& ip, char* name, char* value);
//	bool FindXMLElement(istream& ip, char* name);
	bool FindXMLData(istream& ip, char* data, int maxlength);

		bool iXMLUnits(istream& ip, int& units, double& tolerance) {
		FindXMLElement(ip, "SYSTEM");

		char word[64];
		if(FindXMLAttribute(ip, "UNITS", word)) {
			if(!strcmp(word, "mm")) {
				units = MM;
			}
			else if(!strcmp(word, "metres")) {
				units = METRES;
			}
			else if(!strcmp(word, "inches")) {
				units = INCHES;
			}

			if(FindXMLAttribute(ip, "TOLERANCE", word)) {
				tolerance = atof(word);
				return true;
			}
		}
		return false;
	}

	bool FindXMLAttribute(istream& ip, char* name, char* value) {
		char ch;

		return false;
	}

	bool findString(char* name) {


	}

		bool FindXMLData(istream& ip, char* data, int maxlength) {
		char ch;
		while((ch = ip.get()) != EOF) {
			if(ch != '\"') continue;
			int i = 0;
			while((ch = ip.get()) != EOF) {
				if(ch == '\"') {
					data[i] = 0;
					return true;
				}
				if(i == maxlength) return false;
				data[i++] = ch;
			}
			break;
		}
		return false;
	}
#endif

