# Process the SLN:
f_in  = open('HeeksCNC.sln');
f_vc3 = open('HeeksCNC VC2003.sln', 'w');
f_vc5 = open('HeeksCNC VC2005.sln', 'w');

while (True):
    line = f_in.readline();
    if (len(line) == 0) : break;
    
    if (line == 'Microsoft Visual Studio Solution File, Format Version 10.00\n'):
        f_vc3.write('Microsoft Visual Studio Solution File, Format Version 8.00\n');
        f_vc5.write('Microsoft Visual Studio Solution File, Format Version 9.00\n');
    elif (line == '# Visual C++ Express 2008\n'):
        f_vc5.write('# Visual Studio 2005\n');
    elif (line == 'Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "HeeksCNC", "HeeksCNC.vcproj", "{BE9260B2-CF4A-413B-87BE-4AD857278689}"\n'):
        f_vc3.write('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "HeeksCNC", "HeeksCNC VC2003.vcproj", "{BE9260B2-CF4A-413B-87BE-4AD857278689}"\n');
        f_vc5.write('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "HeeksCNC", "HeeksCNC VC2005.vcproj", "{BE9260B2-CF4A-413B-87BE-4AD857278689}"\n');
    elif (line == 'Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "kurve", "..\kurve\kurve.vcproj", "{3D0F040C-630B-4AF8-96A8-57C634D48926}"\n'):
        f_vc3.write('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "kurve", "..\kurve\kurve VC2003.vcproj", "{3D0F040C-630B-4AF8-96A8-57C634D48926}"\n');
        f_vc5.write('Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "kurve", "..\kurve\kurve VC2005.vcproj", "{3D0F040C-630B-4AF8-96A8-57C634D48926}"\n');

    else:
        f_vc3.write(line);
        f_vc5.write(line);

f_in.close();
f_vc3.close();
f_vc5.close();

# Process the VCPROJ:
f_in  = open('HeeksCNC.vcproj');
f_vc3 = open('HeeksCNC VC2003.vcproj', 'w');
f_vc5 = open('HeeksCNC VC2005.vcproj', 'w');

while (True):
    line = f_in.readline();
    if (len(line) == 0) : break;

    if (line == '\tVersion="9.00"\n'):
        f_vc3.write('\tVersion="7.10"\n');
        f_vc5.write('\tVersion="8.00"\n');
    else:
        line = line.replace('tinyxml.lib', 'tinyxml2005.lib')
        f_vc3.write(line);
        f_vc5.write(line);

f_in.close();
f_vc3.close();
f_vc5.close();

# Process kurve VCPROJ:
f_in  = open('..\kurve\kurve.vcproj');
f_vc3 = open('..\kurve\kurve VC2003.vcproj', 'w');
f_vc5 = open('..\kurve\kurve VC2005.vcproj', 'w');

while (True):
    line = f_in.readline();
    if (len(line) == 0) : break;

    if (line == '\tVersion="9.00"\n'):
        f_vc3.write('\tVersion="7.10"\n');
        f_vc5.write('\tVersion="8.00"\n');
    else:
        f_vc3.write(line);
        f_vc5.write(line);

f_in.close();
f_vc3.close();
f_vc5.close();
