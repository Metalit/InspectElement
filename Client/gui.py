from os import environ
# environ["KIVY_NO_CONSOLELOG"] = "1"

import traceback

from client import start_client, stop_client, send_message

from kivy.app import App
from kivy.properties import (
    NumericProperty, StringProperty, AliasProperty, ReferenceListProperty,
    ObjectProperty, ListProperty, DictProperty, BooleanProperty)
from kivy.clock import Clock
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.stacklayout import StackLayout
from kivy.uix.anchorlayout import AnchorLayout
from kivy.uix.button import Button

def make_field_cell(parent, first_index, has_set, field_type, field_name):
    cell = MethodCell()

    # this way run will call the set method without having to check
    cell.index = first_index + 1
    cell.is_field = True
    cell.field_has_set = has_set
    cell.ids.run_button.text = "Get"
    cell.ids.run_button.bind(on_press=cell.get_field)
    if has_set:
        pad = PaddingCell(padding_vertical=2.5, width=50)
        pad.add_widget(Button(text="Set", on_press=cell.run))
        cell.add_widget(pad)

    cell.add_widget(CompactLabel(text=field_type, padding_horizontal=0))
    cell.add_widget(CompactLabel(text=field_name))

    if has_set:
        field_input = CompactInput(padding_horizontal=0, padding_vertical=2.5)
        cell.add_widget(field_input)
        cell.field_inputs.append(field_input)

    parent.add_widget(cell)
    cell.get_field()

def make_method_cell(parent, method_idx, method_type, method_name, argument_types, argument_names):
    cell = MethodCell()
    
    cell.index = method_idx
    cell.ids.run_button.bind(on_press=cell.run)

    cell.add_widget(CompactLabel(text=method_type, padding_horizontal=0))
    cell.add_widget(CompactLabel(text=method_name))
    cell.add_widget(CompactLabel(text="(", padding_horizontal=0))

    for typ, name in zip(argument_types, argument_names):
        cell.add_widget(CompactLabel(text=typ, padding_horizontal=0))
        cell.add_widget(CompactLabel(text=name))
        field_input = CompactInput(padding_vertical=2.5)
        cell.add_widget(field_input)
        cell.field_inputs.append(field_input)

    cell.add_widget(CompactLabel(text=")", padding_horizontal=0))

    parent.add_widget(cell)

def make_class_region(parent, starting_index, class_name, methods):
    region = ClassDisplay()
    region.ids.class_name.text = class_name

    has_first_field = None
    for i, (is_field, *method) in enumerate(methods):
        # print("adding method", method[1], starting_index + i)
        if is_field:
            if has_first_field == method[1]: # set method of a field
                make_field_cell(region, starting_index + i - 1, True, method[0], method[1])
                has_first_field = None
            elif has_first_field is not None: # new field after a field with only a get method
                make_field_cell(region, starting_index + i - 1, False, methods[i-1][1], methods[i-1][2])
                has_first_field = method[1]
            else:
                has_first_field = method[1] # new field, set to field name
        else:
            if has_first_field is not None: # new method after a field with only a get method
                make_field_cell(region, starting_index + i - 1, False, methods[i-1][1], methods[i-1][2])
                has_first_field = None
            # new method
            make_method_cell(region, starting_index + i, *method)
    
    parent.add_widget(region)
    return starting_index + len(methods)

def sanitize_string(string):
    if not string:
        return " "
    return string.replace("\n", "\\n")

class PaddingCell(AnchorLayout):
    padding_horizontal = NumericProperty(5)
    padding_vertical = NumericProperty(5)

    widget = None

    def callback_size(self, *_):
        self.widget.width = self.width - self.padding_horizontal * 2
        self.widget.height = self.height - self.padding_vertical * 2

    def add_widget(self, widget, index=0, canvas=None):
        if(self.widget):
            raise Exception("Only one widget for a padding cell")
        self.widget = widget
        widget.size_hint = (None, None)
        self.bind(size=self.callback_size)
        return super().add_widget(widget, index=index, canvas=canvas)

class CompactLabel(PaddingCell):
    label = ObjectProperty(None)
    text = StringProperty("")

class CompactButton(Button):
    pass

class CompactInput(PaddingCell):
    text_input = ObjectProperty(None)
    text = StringProperty("")

class ExpandingStackV(StackLayout):
    def callback_size(self, *_):
        height = 0
        for widget in self.children:
            height += widget.height
        self.height = height

    def add_widget(self, widget, *args):
        widget.size_hint_y = None
        self.height += widget.height
        widget.bind(size=self.callback_size)
        return super().add_widget(widget, *args)
    
    def remove_widget(self, widget, *args):
        self.height -= widget.height
        widget.unbind(size=self.callback_size)
        return super().remove_widget(widget, *args)
    
    def __init__(self, **kwargs):
        self.orientation = "tb-lr"
        self.size_hint_y = None
        super().__init__(**kwargs)

class ExpandingStackH(StackLayout):
    def callback_size(self, *_):
        width = 0
        for widget in self.children:
            width += widget.width
        self.width = width

    def add_widget(self, widget, *args):
        widget.size_hint_x = None
        self.width += widget.width
        widget.bind(size=self.callback_size)
        return super().add_widget(widget, *args)
    
    def remove_widget(self, widget, *args):
        self.width -= widget.width
        widget.unbind(size=self.callback_size)
        return super().remove_widget(widget, *args)

    def __init__(self, **kwargs):
        self.orientation = "lr-tb"
        self.size_hint_x = None
        super().__init__(**kwargs)

class MethodCell(ExpandingStackH):
    index = -1
    is_field = False
    field_has_set = False
    field_inputs = ListProperty(list())
    result = None
    result_value = ""

    def run(self, *_):
        if self.index < 0:
            return
        if self.is_field and not self.field_has_set:
            return
        field_strings = list()
        for field_input in self.field_inputs:
            field_strings.append(sanitize_string(field_input.text))
        App.get_running_app().root.run_method(self, self.index, field_strings)
    
    def get_field(self, *_):
        if self.index < 0 or not self.is_field:
            return
        App.get_running_app().root.run_method(self, self.index - 1, list())
    
    def set_result(self, class_name, result):
        if self.result:
            self.result_value = result
            if not class_name or class_name == " ":
                self.result.text = result
        else:
            self.add_widget(CompactLabel(text="="))
            self.result_value = result
            if class_name and class_name != " ":
                self.result = CompactButton(text=class_name, on_press=self.load_result)
            else:
                self.result = CompactLabel(text=self.result_value)
            self.add_widget(self.result)
    
    def load_result(self, *_):
        App.get_running_app().root.load_pointer(self.result_value)

class HistoryCell(Button):
    text = StringProperty("")
    value = StringProperty("")

    def load(self, *_):
        App.get_running_app().root.load_history(self)

class ClassDisplay(ExpandingStackV):
    collapsed = BooleanProperty(False)
    content = ObjectProperty(None)

    def toggle_collapsed(self, *_):
        toggled_collapse = not self.collapsed
        self.ids.expand_button.text = "+" if toggled_collapse else "-"
        if toggled_collapse:
            self.remove_widget(self.content)
        else:
            self.add_widget(self.content)
        self.collapsed = toggled_collapse
        
    def add_widget(self, widget, *args):
        # forward all added widgets to content
        if self.content and widget is not self.content:
            return self.content.add_widget(widget, *args)
        return super().add_widget(widget, *args)
        
    def remove_widget(self, widget, *args):
        # forward all removed widgets to content
        if self.content and widget is not self.content:
            return self.content.remove_widget(widget, *args)
        return super().remove_widget(widget, *args)

    def ClassDisplay_setup(self, *_):
        self.ids.expand_button.bind(on_press=self.toggle_collapsed)
        self.ids.expand_button.text = "+" if self.collapsed else "-"
        if self.collapsed:
            self.remove_widget(self.content)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_once(self.ClassDisplay_setup)

class MainWidget(FloatLayout):
    connected = False
    current_string = ""
    # queue for cell results
    awaiting_cells = list()
    # queue for receiving commands
    processing_command = False
    queued_commands = list()

    history_cells = list()
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_interval(self.check_message, 0.025)

    def connect(self):
        if self.connected:
            stop_client()
            self.ids.connect_button.text = "Connect"
            self.connected = False
            self.ids.methods.clear_widgets()
        else:
            self.connected = start_client(self.ids.ip_input.text, self.receive_message)
            if self.connected:
                self.ids.connect_button.text = "Disconnect"

    def receive_message(self, message):
        self.current_string += message
        str_arr = self.current_string.split("\n"*5)
        self.current_string = str_arr.pop(-1)
        for command in str_arr:
            self.queued_commands.append(command)

    # checks for message periodically on main thread
    def check_message(self, *_):
        if self.connected and len(self.queued_commands) > 0 and not self.processing_command:
            self.process_message(self.queued_commands.pop(0))

    def process_message(self, message):
        # special echo case
        if message.find("echo") == 0:
            # delimiters stripped by reception
            send_message(message[4:] + "\n"*5)
            return
        idx = message.find("\n"*4)
        if idx >= 0:
            command = message[:idx]
            message = message[idx + 4:]
            if command == "result":
                self.process_result(message)
                return
            if command == "class_info":
                self.process_class_info(message)
                return
        print("Could not process message")
    
    def process_result(self, message):
        if len(self.awaiting_cells) <= 0:
            return
        self.processing_command = True
        class_name, result = message.split("\n"*4)
        self.awaiting_cells.pop(0).set_result(class_name, result)
        self.processing_command = False

    def process_class_info(self, message):
        self.processing_command = True
        # throw away anything related to the old object
        self.ids.methods.clear_widgets()
        self.awaiting_cells.clear()
        self.queued_commands.clear()
        try:
            index = 0
            obj_pointer, *class_list = message.split("\n"*4)
            base_type_name = ""
            # create class areas
            for class_string in class_list:
                type_name, *method_list = class_string.split("\n"*3)
                if not base_type_name:
                    base_type_name = type_name
                methods = list()
                # create method and field cells in class areas
                for method_string in method_list:
                    is_field, type_strings, name_strings = method_string.split("\n"*2)
                    is_field = bool(int(is_field))
                    method_type, *param_types = type_strings.split("\n")
                    method_name, *param_names = name_strings.split("\n")
                    methods.append([is_field, method_type, method_name, param_types, param_names])
                index = make_class_region(self.ids.methods, index, type_name, methods)
            # add to history
            history_cell = HistoryCell(text=base_type_name, value=obj_pointer)
            history_cell.bind(on_press=history_cell.load)
            self.history_cells.append(history_cell)
            self.ids.history.add_widget(history_cell)
        except ValueError:
            print("Could not parse class_info")
            # throw away anything that might have been constructed before the error
            self.ids.methods.clear_widgets()
            self.awaiting_cells.clear()
            self.queued_commands.clear()
            self.processing_command = False
            return
        self.processing_command = False
    
    def run_method(self, cell, index, method_args):
        if not self.connected: return

        message_string = "run" + "\n"*4
        message_string += sanitize_string(str(index))
        message_string += "\n"*4
        for method_arg in method_args:
            # adding constructor option later
            message_string += "0" + "\n"*3
            message_string += sanitize_string(method_arg)
            message_string += "\n"*4
        message_string += "\n"
        
        self.awaiting_cells.append(cell)
        send_message(message_string)
    
    def find(self):
        if not self.connected: return

        message_string = "find" + "\n"*4
        message_string += "1" if self.ids.name_find.text else "0"
        message_string += "\n"*4
        message_string += sanitize_string(self.ids.name_find.text)
        message_string += "\n"*4
        message_string += sanitize_string(self.ids.namespace_find.text)
        message_string += "\n"*4
        message_string += sanitize_string(self.ids.class_find.text)
        message_string += "\n"*5

        send_message(message_string)
    
    def load_history(self, cell):
        if not self.history_cells.count(cell):
            self.ids.history.remove_widget(cell)
            self.load_pointer(cell.value)
            return
        idx = self.history_cells.index(cell)
        for later_cell in self.history_cells[idx:]:
            self.ids.history.remove_widget(later_cell)
        self.history_cells = self.history_cells[:idx]
        self.load_pointer(cell.value)
    
    def load_pointer(self, pointer):
        if not self.connected: return

        message_string = "load" + "\n"*4 + pointer + "\n"*5
        send_message(message_string)
    
    def clear_history(self, *_):
        for cell in self.history_cells:
            self.ids.history.remove_widget(cell)
        self.history_cells = list()

class GUIApp(App):
    def build(self):
        self.title = "Inspect Element Client"
        return MainWidget()

    def on_stop(self):
        stop_client()
        super().on_stop()

if __name__ == '__main__':
    try:
        GUIApp().run()
    except Exception as e: # yes, ALL exceptions
        traceback.print_exc()
        stop_client()