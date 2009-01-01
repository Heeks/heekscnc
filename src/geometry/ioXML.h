// written by g.j.hawkesford 2006 for Camtek Gmbh

// simple XML output classes to implement sufficient for p4c/peps purposes
// 
#pragma once

	class inputXML {
	private:
		wifstream* ip;
		double scale;

	public:
		inputXML(const std::wstring& fname);
		inputXML(const wchar_t* fname);
		~inputXML();
		bool startElement(const wchar_t* name);
		bool startElement(const std::wstring& name);
		bool endElement();
		bool findString(const std::wstring& name);
		bool Attribute(const wchar_t* name, int& data);
		bool Attribute(const wchar_t* name, bool& data);
		bool Attribute(const wchar_t* name, wchar_t* data);
		bool Attribute(const wchar_t* name, std::wstring& data);
		bool Attribute(const wchar_t* name, double& data, bool scaledata = true);
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
		outXML(const wchar_t* fname, bool m_scaleToMM = false);
		outXML(const std::wstring& fname, bool m_scaleToMM = false);
		~outXML();
		void Write(const Point& p, const wchar_t* name, double scale = 1.0);
		void Write(const Point3d& p, const wchar_t* name);
		void Write(const Vector2d& v, const wchar_t* name);
		void Write(const Vector3d& v, const wchar_t* name);
		void Write(const CLine& cl, const wchar_t* name);
		void Write(const Circle& c, const wchar_t* name);
		void Write(const Matrix& m, const wchar_t* name);
		void Write(const Kurve& k, const wchar_t* name, double scale = 1.0);
		void Write(const spVertex& sp, const wchar_t* name = L"", double scale = 1.0);
		void startElement(const wchar_t* ele);
		void startElement(const std::wstring& ele);
		void endElement();
		void Attribute(const std::wstring& att, int data);
		void Attribute(const wchar_t* att, int data);
		void Attribute(const std::wstring& att, bool data);
		void Attribute(const wchar_t* att, bool data);
		void Attribute(const std::wstring& att, double data, bool scaleData = true);
		void Attribute(const wchar_t* att, double data, bool scaleData = true);
		void Attribute(const std::wstring& att, const std::wstring& data);
//		void Attribute(wchar_t* att, char* data);
		void Attribute(const wchar_t* att, const wchar_t* data);
		void Attribute(const std::wstring& att, const wchar_t* data);
		void Attribute(const wchar_t* att, std::wstring data);
		void Attribute(const wchar_t* att, const void* data);
		void Attribute(const Point& p);
		void Attribute(const Point3d& p);
		void Units(void);
		void Comment(const wchar_t* comment);
		void Body(const wchar_t* bodytext);
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

