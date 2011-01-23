import HeeksCNC

class MachineState:
    def __init__(self):
        self.tool_number = 0
        
    def AppendToolChangeText(self, new_tool_number):
        if self.tool_number != new_tool_number:
            self.tool_number = new_tool_number;

            # Select the right tool.
            tool = HeeksCNC.program.tools.FindTool(new_tool_number)
            if tool != None:
                HeeksCNC.program.python_program += "comment('tool change to " + tool.title + "')\n"
                HeeksCNC.program.python_program += "tool_change( id=" + str(new_tool_number) + ")\n"
                #if(m_attached_to_surface)
                #{
                #   python << _T("nc.nc.creator.cutter = ") << pTool->OCLDefinition() << _T("\n");
                #}
