// GTri.h

// triangle used for Anders's DropCutter code
// written by Dan Heeks starting on May 2nd 2008

class GTri{
public:
	double m_p[9]; // three points
	double m_n[3]; // normal, calculate this when loading stl file ( or creating from solid )
	double m_box[4]; // minx miny maxx maxy

	GTri(const double* x){memcpy(m_p, x, 9*sizeof(double)); calculate_box_and_normal();}

	void calculate_box_and_normal(){
		double v1[3] = {m_p[3] - m_p[0], m_p[4] - m_p[1], m_p[5] - m_p[2]};
		double v2[3] = {m_p[6] - m_p[0], m_p[7] - m_p[1], m_p[8] - m_p[2]};

		m_n[0] = v1[1] * v2[2] - v1[2] * v2[1];
		m_n[1] = v1[2] * v2[0] - v1[0] * v2[2];
		m_n[2] = v1[0] * v2[1] - v1[1] * v2[0];

		// normalise it
		double m = sqrt(m_n[0] * m_n[0] + m_n[1] * m_n[1] + m_n[2] * m_n[2]);
		if(m > 0.000000001)
		{
			m_n[0] /= m;
			m_n[1] /= m;
			m_n[2] /= m;
		}

		m_box[0] = m_p[0];
		if(m_p[3] < m_box[0])m_box[0] = m_p[3];
		if(m_p[6] < m_box[0])m_box[0] = m_p[6];
		m_box[1] = m_p[1];
		if(m_p[4] < m_box[1])m_box[1] = m_p[4];
		if(m_p[7] < m_box[1])m_box[1] = m_p[7];
		m_box[2] = m_p[0];
		if(m_p[3] > m_box[2])m_box[2] = m_p[3];
		if(m_p[6] > m_box[2])m_box[2] = m_p[6];
		m_box[3] = m_p[1];
		if(m_p[4] > m_box[3])m_box[3] = m_p[4];
		if(m_p[7] > m_box[3])m_box[3] = m_p[7];
	}

	static bool box_in_box(double *this_box, double *box){
		if(this_box[0]<box[0]-heeksCAD->GetTolerance()){
			// left of tri is left of box
			if(this_box[2]<box[0]-heeksCAD->GetTolerance()){
				// right of tri is left of box
				return false;
			}
			else if(this_box[2]<box[2] + heeksCAD->GetTolerance()){
				// right of tri is in box
				if(this_box[1]<box[1]-heeksCAD->GetTolerance()){
					// bottom of tri is below box
					if(this_box[3]<box[1]-heeksCAD->GetTolerance()){
						// top of tri is below of box
						return false;
					}
					else{
						// top of tri is in box or above it
						return true;
					}
				}
				else if(this_box[1]<box[3]+heeksCAD->GetTolerance()){
					// bottom of tri is in box
					return true;
				}
				else{
					// bottom of tri is above box
					return false;
				}
			}
			else{
				// right of tri is right of box
				if(this_box[1]>box[1]-heeksCAD->GetTolerance() && this_box[3]<box[3]+heeksCAD->GetTolerance()){
					// top and bottom within box
					return true;
				}
				else{
					return false;
				}
			}
		}
		else if(this_box[0]<box[2]+heeksCAD->GetTolerance()){
			// left of tri is within box
			if(this_box[1]<box[1]-heeksCAD->GetTolerance()){
				// bottom of tri is below box
				if(this_box[3]<box[1]-heeksCAD->GetTolerance()){
					// top of tri is below of box
					return false;
				}
				else{
					// top of tri is in box or above it
					return true;
				}
			}
			else if(this_box[1]<box[3]+heeksCAD->GetTolerance()){
				// bottom of tri is in box
				return true;
			}
			else{
				// bottom of tri is above box
				return false;
			}
		}
		else{
			// left of tri is right of box
			return false;
		}
	}
};

