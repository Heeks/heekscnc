#include <iostream>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <map>

#include "juce.h"

using namespace std;


double dist(double a,double b)
{
    if (a < b) return(b-a);
    else return(a-b);
}

/**
    This program was written so that a CNC machine could be used to
    produce paper tapes for a music box such as http://kikkerland.com/S06/1200.htm

    These music boxes accept a paper tape with holes punched where notes
    are to be played.

    This program accepts a MIDI filename as a command line argument
    and generates a HeeksCAD file representing it.  Lines and points
    are placed for the various notes that make up the tune.

    Separate sets of lines/points are generated for each track within
    the MIDI file.

    Text is overlayed if the MIDI file has words included (as a text track)

    The points placed in the HeeksCAD file can then be used as drilling locations
    for the generation of a GCode program.
 */

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage %s file.mid\n", argv[0]);
        return(-1);
    }

    const char *l_pszFileName = argv[1];
    File file(l_pszFileName);

    const double mm_per_second = 1000.0 / 60.0;  // How fast does the tape move through the music box? (1 meter per minute - by observation)
    const double distance_between_notes_in_mm = 2.0; // across the paper.
    const double min_distance_between_notes = 7.0;  // Cannot have two consecutive notes appear less than this distance between each other.

    std::list<std::string> gcode;
    std::list<std::string> heeks;

    int id=1;

    typedef std::vector<std::string> Keys_t;
    Keys_t keys, legal_keys;
    Keys_t::size_type middle_c_key;

    // Which integer tells us it's the 'C' in the middle or the 'C' in the
    // octave above or below.
    const int middle_c_octave = 5;

    for (int octave=2; octave <= 8; octave++)
    {
        for (char key='A'; key<='G'; key++)
        {
            std::ostringstream l_ossKey;

            // l_ossKey << key << middle_c_octave - 0;
            l_ossKey << key << octave;
            keys.push_back( l_ossKey.str() );

            if ((key == 'C') && (octave == middle_c_octave)) middle_c_key = keys.size()-1;
        }
    }

    // Setup our scale of notes that will work with the music box.  It covers from 'C' to 'C' over two octaves.
    // Octave below middle C
    for (char key='C'; key<='G'; key++)
    {
        std::ostringstream l_ossKey;

        l_ossKey << key << middle_c_octave - 1;
        legal_keys.push_back( l_ossKey.str() );
    }

    // Octave that includes middle C
    for (char key='A'; key<='G'; key++)
    {
        std::ostringstream l_ossKey;

        l_ossKey << key << middle_c_octave - 0;
        legal_keys.push_back( l_ossKey.str() );
    }

    // Octave above middle C
    for (char key='A'; key<='C'; key++)
    {
        std::ostringstream l_ossKey;

        l_ossKey << key << middle_c_octave + 1;
        legal_keys.push_back( l_ossKey.str() );
    }


    const double track_width = distance_between_notes_in_mm * keys.size();
    const double space_between_tracks = track_width * 0.75;

    MidiFile midi_file;
    FileInputStream midi_input_stream(file);

    if (! midi_file.readFrom( midi_input_stream ))
    {
        fprintf(stderr,"Could not open '%s' for reading\n", l_pszFileName);
        return(-1);
    }

    midi_file.convertTimestampTicksToSeconds();
    std::set<int> notes;

    double time_scale = 1.0;
    bool time_scale_changed = false;

    do {
        std::map<std::string, double>   key_position;
        time_scale_changed = false;
        gcode.clear();
        heeks.clear();
        key_position.clear();

        std::ostringstream l_ossGCode;

        for (int track = 0; track<midi_file.getNumTracks(); track++)
        {
            int number_of_notes_included = 0;
            int number_of_notes_ignored = 0;

            const MidiMessageSequence *pMessageSequence = midi_file.getTrack(track);
            double start_time = pMessageSequence->getStartTime();
            double end_time = pMessageSequence->getEndTime();
            double duration = end_time - start_time;

            if (duration <= 0.0001) continue;

            l_ossGCode.str("");
            l_ossGCode << "(Duration of track " << track << " is " << duration << " seconds)";
            gcode.push_back( l_ossGCode.str() );
            printf("%s\n", l_ossGCode.str().c_str());

            // printf("Duration of track %d is %lf seconds\n", track, duration);

            for (int event = 0; event < pMessageSequence->getNumEvents(); event++)
            {
                MidiMessageSequence::MidiEventHolder *pEvent = pMessageSequence->getEventPointer(event);
                MidiMessage message = pEvent->message;
                double time_stamp = message.getTimeStamp();

                if (message.isTextMetaEvent())
                {
                    String text = message.getTextFromTextMetaEvent();
                    char buf[1024];
                    memset( buf, '\0', sizeof(buf) );
                    text.copyToBuffer( buf, sizeof(buf)-1 );
                    // printf("Track %d is %s\n", track, buf );
                    l_ossGCode.str("");
                    l_ossGCode << "(Text track " << track << " is " << buf << ")";
                    gcode.push_back(l_ossGCode.str());
                    printf("%s\n", l_ossGCode.str().c_str());

                    std::ostringstream l_ossHeeks;
                    l_ossHeeks << "<Text text=\"" << buf << "\" font=\"OpenGL\" col=\"0\" m0=\"-0.0443342566\" m1=\"-0.999016753\" m2=\"0\" m3=\"" << (double) (time_stamp * mm_per_second) << "\" m4=\"0.999016753\" m5=\"-0.0443342566\" m6=\"0\" m7=\"" << (double) ((track_width + space_between_tracks) * track) << "\" m8=\"0\" m9=\"0\" ma=\"1\" mb=\"0\" id=\"" << id++ << "\" />";
                    heeks.push_back( l_ossHeeks.str() );
                }

                if (message.isTrackNameEvent())
                {
                    String text = message.getTextFromTextMetaEvent();
                    char buf[1024];
                    memset( buf, '\0', sizeof(buf) );
                    text.copyToBuffer( buf, sizeof(buf)-1 );
                    printf("Track %d is %s\n", track, buf );
                }

                if (message.isNoteOn())
                {
                    char note_name[256];
                    memset( note_name, '\0', sizeof(note_name) );
                    message.getMidiNoteName(message.getNoteNumber(), true, true, middle_c_octave).copyToBuffer( note_name, sizeof(note_name)-1 );

                    notes.insert( message.getNoteNumber() );

                    // printf("time %lf note %s\n", time_stamp, note_name );
                    std::string l_ssNoteName(note_name);
                    std::string::size_type offset;
                    bool sharp_found = false;
                    while ((offset = l_ssNoteName.find("#")) != std::string::npos)
                    {
                        l_ssNoteName = l_ssNoteName.erase(offset,1);
                        sharp_found = true;
                    }

                    strncpy( note_name, l_ssNoteName.c_str(), sizeof(note_name)-1 );

                    const int blue = 16711680;
                    const int black = 0;
                    const int red = 255;

                    int colour = blue;
                    Keys_t::iterator l_itLegalKey = std::find( legal_keys.begin(), legal_keys.end(), note_name );
                    if (l_itLegalKey == legal_keys.end())
                    {
                        colour = red;
                    }

                    // Find the note name in the keys we're interested in.
                    Keys_t::iterator l_itKey = std::find( keys.begin(), keys.end(), note_name );
                    if (l_itKey != keys.end())
                    {
                        double x = time_stamp * mm_per_second * time_scale;
                        double y = double(double(std::distance( keys.begin(), l_itKey )) - double(middle_c_key)) * distance_between_notes_in_mm;

                        y += ((track_width + space_between_tracks) * track);

                        if (sharp_found)
                        {
                            y += (distance_between_notes_in_mm / 2.0);
                            colour = red;
                        }

                        // Check to see if we have two notes that are too close to each other for the mechanism to play them.
                        if (key_position.find(note_name) == key_position.end())
                        {
                            key_position[note_name] = x;
                        }


                        // Measure the distance between this note and the previous equivalent note.  If we need to expand our
                        // time scale to ensure consecutive notes are not too close together, do it now.
                        if ((dist(x, key_position[note_name]) < min_distance_between_notes) && (dist(x, key_position[note_name]) > 0.0))
                        {
                            // Need to scale the whole piece up.
                            double increase_in_time_scale = double(double(min_distance_between_notes) / double(dist(x, key_position[note_name])));

                            if (increase_in_time_scale > 1.0)
                            {
                                time_scale = increase_in_time_scale * time_scale;
                                time_scale_changed = true;
                            }
                        }

                        key_position[note_name] = x;

                        // It's a key we have to play.  Generate the GCode.
                        l_ossGCode.str("");
                        l_ossGCode << "G83 X " << x << " Y " << y << "\t(" << note_name << ")";
                        gcode.push_back( l_ossGCode.str() );

                        if (sharp_found)
                        {
                            std::ostringstream l_ossHeeks;
                            l_ossHeeks << "<Circle col=\"" << colour << "\" r=\"" << (distance_between_notes_in_mm / 2.0) * 0.85 << "\" cx=\"" << x << "\" cy=\"" << y << "\" cz=\"0\" ax=\"0\" ay=\"0\" az=\"1\" id=\"" << id++ << "\">\n";
                            l_ossHeeks << "   <Point col=\"" << colour << "\" x=\"" << x << "\" y=\"" << y << "\" z=\"0\" id=\"" << id++ << "\" />\n";
                            l_ossHeeks << "</Circle>\n";
                            heeks.push_back( l_ossHeeks.str() );
                        }
                        else
                        {
                            std::ostringstream l_ossHeeks;
                            l_ossHeeks << "<Point col=\"" << colour << "\" x=\"" << x << "\" y=\"" << y << "\" z=\"0\" id=\"" << id++ << "\" />";
                            heeks.push_back( l_ossHeeks.str() );
                        }

                        // printf("G83 Want hole for key %s at %lf,%lf\n", note_name, x, y );
                        number_of_notes_included++;
                    }
                    else
                    {
                        // This key doesn't fall exactly on our scale.  Ignore it.
                        number_of_notes_ignored++;
                        printf("Missed note %s\n", note_name);
                    }
                } // End if - then
            } // End for


            l_ossGCode.str("");
            l_ossGCode << "(" << (double(number_of_notes_included)/double(number_of_notes_included + number_of_notes_ignored)) * 100.0
                            << " % utilisation of notes)";
            gcode.push_back(l_ossGCode.str());
            printf("%s\n", l_ossGCode.str().c_str());

            l_ossGCode.str("");
            l_ossGCode << "(Of the " << (number_of_notes_included + number_of_notes_ignored) << " notes, we are using "
                            << number_of_notes_included << " (whole notes) and ignoring " << number_of_notes_ignored << " (sharps and flats))";
            gcode.push_back(l_ossGCode.str());
            printf("%s\n", l_ossGCode.str().c_str());

            printf("At %lf mm per second (%lf mm per minute), we will need %lf mm of paper for this tune\n",
                    mm_per_second, mm_per_second * 60.0 * time_scale, (end_time - start_time) * mm_per_second * time_scale );

            printf("We have had to scale the tune %lf times to ensure no two consecutive notes were less than %lf mm apart\n",
                time_scale, min_distance_between_notes);

            // Draw a line for each possible note.
            for (Keys_t::iterator l_itKey = keys.begin(); l_itKey != keys.end(); l_itKey++)
            {
                double y = double(double(std::distance( keys.begin(), l_itKey )) - double(middle_c_key)) * distance_between_notes_in_mm;

                y += ((track_width + space_between_tracks) * track);

                if (std::find(legal_keys.begin(), legal_keys.end(), *l_itKey) != legal_keys.end())
                {
                    std::ostringstream l_ossHeeks;
                    l_ossHeeks.str("");
                    l_ossHeeks << "<Sketch title=\"Sketch\" id=\"" << id++ << "\">\n";
                    l_ossHeeks << "<Line col=\"0\" id=\"" << id++ << "\">\n";
                    l_ossHeeks << "<Point col=\"0\" x=\"" << (double) (start_time * mm_per_second * time_scale) << "\" y=\"" << y << "\" z=\"0\" id=\"" << id++ << "\" />\n";
                    l_ossHeeks << "<Point col=\"0\" x=\"" << (double) (end_time * mm_per_second * time_scale) << "\" y=\"" << y << "\" z=\"0\" id=\"" << id++ << "\" />\n";
                    l_ossHeeks << "</Line>\n";
                    l_ossHeeks << "</Sketch>\n";
                    heeks.push_back(l_ossHeeks.str());
                }
            } // End for
        }
	 } while (time_scale_changed == true);

/*
    for (std::set<int>::const_iterator l_itNote = notes.begin(); l_itNote != notes.end(); l_itNote++)
    {
        char note_name[256];
        memset( note_name, '\0', sizeof(note_name) );
        MidiMessage::getMidiNoteName(*l_itNote, true, true, 5).copyToBuffer( note_name, sizeof(note_name)-1 );

        printf("Note %d %s\n", *l_itNote, note_name);
    }
*/

    {
        String gcode_file_name(l_pszFileName);
        gcode_file_name = gcode_file_name.dropLastCharacters(4);
        gcode_file_name << ".ngc";
        char buf[1024];
        memset( buf, '\0', sizeof(buf) );
        gcode_file_name.copyToBuffer(buf,sizeof(buf)-1);
        FILE *fp = fopen( buf, "w+t");
        if (fp == NULL)
        {
            fprintf(stderr,"Could not open %s for writing\n", buf);
            return(-1);
        }
        for (std::list<std::string>::const_iterator l_itLine = gcode.begin(); l_itLine != gcode.end(); l_itLine++)
        {
            fprintf(fp,"%s\n", l_itLine->c_str());
        }
        fclose(fp);
    }

    {
        String gcode_file_name(l_pszFileName);
        gcode_file_name = gcode_file_name.dropLastCharacters(4);
        gcode_file_name << ".heeks";
        char buf[1024];
        memset( buf, '\0', sizeof(buf) );
        gcode_file_name.copyToBuffer(buf,sizeof(buf)-1);
        FILE *fp = fopen( buf, "w+t");
        if (fp == NULL)
        {
            fprintf(stderr,"Could not open %s for writing\n", buf);
            return(-1);
        }
        for (std::list<std::string>::const_iterator l_itLine = heeks.begin(); l_itLine != heeks.end(); l_itLine++)
        {
            fprintf(fp,"%s\n", l_itLine->c_str());
        }

        fclose(fp);
    }


    return 0;
}

