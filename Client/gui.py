from os import environ
# environ["KIVY_NO_CONSOLELOG"] = "1"

from client import start_client, stop_client

from kivy.app import App
from kivy.uix.widget import Widget
from kivy.properties import (
    NumericProperty, StringProperty, AliasProperty, ReferenceListProperty,
    ObjectProperty, ListProperty, DictProperty, BooleanProperty)
from kivy.clock import Clock
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.stacklayout import StackLayout
from kivy.uix.anchorlayout import AnchorLayout
from kivy.uix.label import Label
from kivy.uix.textinput import TextInput

def makeMethodCell(parent, methodIdx, methodTyp, methodName, argTyps, argNames):
    print("making cell")
    cell = MethodCell()
    
    cell.index = methodIdx
    cell.ids.run_button.bind(on_press=cell.run)

    cell.add_widget(CompactLabel(text=methodTyp))
    cell.add_widget(CompactLabel(text=methodName))

    for typ, name in zip(argTyps, argNames):
        cell.add_widget(CompactLabel(text=typ))
        cell.add_widget(CompactLabel(text=name))
        cell.add_widget(FieldInput())

    parent.add_widget(cell)

def makeClassRegion(parent, startingIndex, className, methods):
    region = ClassDisplay()
    region.ids.class_name.text = className

    for i, method in enumerate(methods):
        makeMethodCell(region, startingIndex + i, *method)
    
    parent.add_widget(region)
    return startingIndex + len(methods)

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

class CompactLabel(Label):
    pass

class FieldInput(TextInput):
    pass

class ExpandingStack(StackLayout):
    isVertical = True
    verticalSet = False

    def callback_size(self, *args):
        if self.verticalSet:
            if self.isVertical:
                height = 0
                for widget in self.children:
                    height += widget.height
                self.height = height
            else:
                width = 0
                for widget in self.children:
                    width += widget.width
                self.width = width

    def add_widget(self, widget, *args):
        if self.verticalSet:
            if self.isVertical:
                widget.size_hint_y = None
                self.height += widget.height
            else:
                widget.size_hint_x = None
                self.width += widget.width
        widget.bind(size=self.callback_size)
        return super().add_widget(widget, *args)
    
    def remove_widget(self, widget, *args):
        if self.verticalSet:
            if self.isVertical:
                self.height -= widget.height
            else:
                self.width -= widget.width
        widget.unbind(size=self.callback_size)
        return super().remove_widget(widget, *args)

    def ExpandingStack_setup(self, *args):
        self.isVertical = self.orientation[:2] == "tb" or self.orientation[:2] == "bt"
        self.verticalSet = True
        if self.isVertical:
            self.size_hint_y = None
            for widget in self.children:
                widget.size_hint_y = None
        else:
            self.size_hint_x = None
            for widget in self.children:
                widget.size_hint_x = None
        self.callback_size()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_once(self.ExpandingStack_setup)

class MethodCell(ExpandingStack):
    index = -1
    fieldInputs = ListProperty(list())

    def run(self, *args):
        if self.index < 0:
            return
        run_string = self.index.ToString()
        print("run", run_string)

class ClassDisplay(ExpandingStack):
    collapsed = BooleanProperty(False)
    content = ObjectProperty(None)

    def toggle_collapsed(self, *args):
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

    def ClassDisplay_setup(self, *args):
        self.ids.expand_button.bind(on_press=self.toggle_collapsed)
        self.ids.expand_button.text = "+" if self.collapsed else "-"
        if self.collapsed:
            self.remove_widget(self.content)

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_once(self.ClassDisplay_setup)

class MainWidget(FloatLayout):
    connected = False
    currentMessage = ""
    currentString = ""
    lastRanCell = None
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        Clock.schedule_interval(self.check_message, 0.5)

    def connect(self):
        if self.connected:
            stop_client()
            self.ids.connect_button.text = "Connect"
            self.connected = False
        else:
            self.connected = start_client(self.ids.ip_input.text, self.receive_message)
            if self.connected:
                self.ids.connect_button.text = "Disconnect"

    def receive_message(self, message):
        self.currentString += message
        idx = self.currentString.find("\n"*5)
        if idx >= 0:
            self.currentMessage = self.currentString[:idx]
            self.currentString = self.currentString[idx + 5:]

    # checks for message periodically on main thread
    def check_message(self, *args):
        if self.connected and self.currentMessage:
            self.process_message(self.currentMessage)
            self.currentMessage = ""

    def process_message(self, message):
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
        pass

    def process_class_info(self, message):
        try:
            index = 0
            obj_pointer, *class_list = message.split("\n"*4)
            for class_string in class_list:
                type_name, *method_list = class_string.split("\n"*3)
                print("Processing class", type_name)
                methods = list()
                for method_string in method_list:
                    is_field, type_strings, name_strings = method_string.split("\n"*2)
                    is_field = bool(int(is_field))
                    method_type, *param_types = type_strings.split("\n")
                    method_name, *param_names = name_strings.split("\n")
                    methods.append([method_type, method_name, param_types, param_names])
                index = makeClassRegion(self.ids.methods, index, type_name, methods)
        except ValueError:
            print("Could not parse class_info")
            return

class GUIApp(App):
    def build(self):
        return MainWidget()

    def on_stop(self):
        stop_client()
        super().on_stop()

if __name__ == '__main__':
    GUIApp().run()