import os
import tkinter as tk
from tkinter import filedialog
from tkinter import messagebox
from tkinter import scrolledtext
import subprocess
from tkinter import ttk
import threading, re

def browse_folder():
    folder_path = filedialog.askdirectory()
    if folder_path:
        list_files_in_directory(folder_path)

def list_files_in_directory(folder_path):
    file_list.delete(*file_list.get_children())  # Clear the existing file list
    file_list.insert("", "end", values=('..', "Folder", folder_path+'/..'))
    for item in os.listdir(folder_path):
        item_path = os.path.join(folder_path, item)
        if os.path.isdir(item_path):
            file_list.insert("", "end", values=(item, "Folder", item_path))
        else:
            if item_path.lower().endswith(('.fbx', '.gltf', '.obj')):
                file_list.insert("", "end", values=(item, "Mesh", item_path))
            elif item_path.lower().endswith(('.txt', '.json')):
                file_list.insert("", "end", values=(item, "Text", item_path))
            else:
                file_list.insert("", "end", values=(item, "File", item_path))

def on_listbox_select(event):
    selected_item = file_list.selection()
    if selected_item:
        item_values = file_list.item(selected_item, "values")
        file_path = item_values[2]
        if item_values[1] == "Folder":
            list_files_in_directory(file_path)
        else:
        # if item_values[1] == "File" or item_values[1] == "Mesh":
            try:
                os.startfile(file_path)
            except OSError:
                messagebox.showinfo("Error", "Cannot open file.")
        # elif item_values[1] == "Text":
            # show_text_file(file_path)

# def on_listbox_select(event):
#     selected_item = files_listbox.curselection()
#     if selected_item:
#         file_or_folder_name = files_listbox.get(selected_item[0])
#         full_path = os.path.join(selected_folder.get(), file_or_folder_name)
#         if file_or_folder_name == "..":
#             current_dir = selected_folder.get()
#             parent_dir = os.path.abspath(os.path.join(current_dir, os.pardir))
#             selected_folder.set(parent_dir)
#             list_files_in_directory(parent_dir)
#         elif os.path.isdir(full_path):
#             selected_folder.set(full_path)
#             list_files_in_directory(full_path)
#         else:
#             if file_or_folder_name.endswith(('.txt', '.json')):
#                 show_text_file(full_path)
#             elif file_or_folder_name.endswith(('.fbx', '.gltf', '.obj')):
#             else:
#                 messagebox.showinfo("Info", "This is not a text file.")

def show_text_file(file_path):
    with open(file_path, 'r') as file:
        content = file.read()

    # Create a new window to display the text file content
    text_viewer_window = tk.Toplevel(app)
    text_viewer_window.title("Text File Viewer")

    text_widget = scrolledtext.ScrolledText(text_viewer_window, wrap=tk.WORD, width=80, height=30)
    text_widget.pack()

    text_widget.insert(tk.END, content)
    text_widget.config(state=tk.DISABLED)

def async_run_export_script(script):
    global process
    def run_script_and_display_output():
        try:
            process = subprocess.Popen(script, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            for line in process.stdout:
                update_output(line, "green")
            for line in process.stderr:
                update_output(line, "red")
        except subprocess.CalledProcessError as e:
            update_output(e.output)

    def update_output(line, color=None):
        # Remove ANSI escape sequences from the output before updating the text widget
        clean_line = re.sub(r'\x1b\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]', '', line)
        output_text_widget.config(state=tk.NORMAL)
        if color:
            apply_ansi_colors(clean_line, color)
        else:
            output_text_widget.insert(tk.END, clean_line)
        output_text_widget.see(tk.END)
        output_text_widget.config(state=tk.DISABLED)

    def apply_ansi_colors(text, color):
        # Apply color tags to the text widget based on ANSI color codes
        output_text_widget.tag_configure(color, foreground=color)
        output_text_widget.insert(tk.END, text, color)

    # Clear the output text widget before running the script
    output_text_widget.config(state=tk.NORMAL)
    output_text_widget.delete('1.0', tk.END)
    output_text_widget.config(state=tk.DISABLED)

    # Start a separate thread to run the script and display the output
    thread = threading.Thread(target=run_script_and_display_output)
    thread.daemon = True
    thread.start()


def strip_colors(text):
    import re
    ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
    return ansi_escape.sub('', text)


# Create the main application window
app = tk.Tk()
app.title("ZYY Engine")

# Styling
style = ttk.Style()
app.tk.call('source', 'forest-dark.tcl')
style.theme_use("forest-dark")

# Create and place the widgets
selected_folder = tk.StringVar()
selected_folder.set(os.path.abspath("res"))

frame = ttk.Frame(app)
frame.pack(padx=20, pady=10)

# folder_label = ttk.Label(frame, text="Selected Folder:")
# folder_label.pack()

# folder_entry = ttk.Entry(frame, textvariable=selected_folder, width=50)
# folder_entry.pack(pady=5)

# folder_button = ttk.Button(frame, text="Browse Folder", command=browse_folder)
# folder_button.pack(pady=5)

# files_listbox = tk.Listbox(frame, width=50, height=15)
# files_listbox.pack()
# files_listbox.bind("<<ListboxSelect>>", on_listbox_select)

# Create the file view using ttk.Treeview
file_list = ttk.Treeview(app, columns=("Type", "Path"), show="headings")
file_list.heading("Type", text="Type")
file_list.heading("Path", text="Path")

# Add an event handler for the file view selection
file_list.bind("<<TreeviewSelect>>", on_listbox_select)
file_list.pack(fill=tk.BOTH, expand=True)

run_button = ttk.Button(frame, text="Run Export Script", command=lambda: async_run_export_script('export_assets.bat'))
run_button.pack(pady=10)
run_button.pack(side=tk.LEFT)

build_button = ttk.Button(frame, text="Build Game", command=lambda: async_run_export_script('build n game'))
build_button.pack(pady=10)
build_button.pack(side=tk.LEFT)

platform_button = ttk.Button(frame, text="Build Platform", command=lambda: async_run_export_script('build win32'))
platform_button.pack(pady=10)
platform_button.pack(side=tk.LEFT)

physics_button = ttk.Button(frame, text="Build Physics", command=lambda: async_run_export_script('build physics'))
physics_button.pack(pady=10)
physics_button.pack(side=tk.LEFT)

launch_button = ttk.Button(frame, text="Launch", command=lambda: async_run_export_script('run'))
launch_button.pack(pady=10)


# Create the output text widget
output_text_widget = scrolledtext.ScrolledText(app, wrap=tk.WORD, width=80, height=15, state=tk.DISABLED)
output_text_widget.pack(padx=20, pady=10)


list_files_in_directory(os.path.abspath("res"))

process = None

def on_close():
    global process
    # Terminate the subprocess and close the application window

    if process:  # Check if the subprocess is still running
        # process.kill()
        print("killing")
        subprocess.Popen("TASKKILL /F /PID {pid} /T".format(pid=process.pid))
    app.destroy()
    
app.protocol("WM_DELETE_WINDOW", on_close)

# Start the application's main event loop
app.mainloop()
