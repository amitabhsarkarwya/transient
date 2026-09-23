using Microsoft.Win32;
using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;

internal static class InstallerProgram
{
    private const string ProductName = "Transient Shaper VST3";
    private const string UninstallKey = @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\TransientShaperVST3";
    private const string InstallPathValue = "InstallPath";

    [STAThread]
    private static void Main(string[] args)
    {
        if (args.Length > 0 && args[0].Equals("--uninstall", StringComparison.OrdinalIgnoreCase))
        {
            Uninstall();
            return;
        }

        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new SetupForm());
    }

    private static string DefaultVst3Folder
    {
        get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "VST3"); }
    }

    private static string ProductFolder
    {
        get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonProgramFiles), "Transient Shaper Installer"); }
    }

    private static void Install(string vst3Folder)
    {
        string bundleFolder = Path.Combine(vst3Folder, "Transient Shaper.vst3");
        string binaryPath = Path.Combine(bundleFolder, "Contents", "x86_64-win", "Transient Shaper.vst3");
        Directory.CreateDirectory(Path.GetDirectoryName(binaryPath));

        using (Stream input = Assembly.GetExecutingAssembly().GetManifestResourceStream("TransientShaperBinary"))
        {
            if (input == null)
                throw new InvalidOperationException("The VST3 plug-in payload is missing from this installer.");
            using (var output = new FileStream(binaryPath, FileMode.Create, FileAccess.Write, FileShare.None))
                input.CopyTo(output);
        }

        Directory.CreateDirectory(ProductFolder);
        string uninstallExe = Path.Combine(ProductFolder, "Uninstall Transient Shaper.exe");
        string currentExe = Assembly.GetExecutingAssembly().Location;
        if (!String.Equals(currentExe, uninstallExe, StringComparison.OrdinalIgnoreCase))
            File.Copy(currentExe, uninstallExe, true);

        using (RegistryKey key = Registry.LocalMachine.CreateSubKey(UninstallKey))
        {
            key.SetValue("DisplayName", ProductName, RegistryValueKind.String);
            key.SetValue("DisplayVersion", "1.0.0", RegistryValueKind.String);
            key.SetValue("Publisher", "Transient Shaper", RegistryValueKind.String);
            key.SetValue(InstallPathValue, vst3Folder, RegistryValueKind.String);
            key.SetValue("UninstallString", "\"" + uninstallExe + "\" --uninstall", RegistryValueKind.String);
            key.SetValue("NoModify", 1, RegistryValueKind.DWord);
            key.SetValue("NoRepair", 1, RegistryValueKind.DWord);
        }
    }

    private static void Uninstall()
    {
        string vst3Folder = null;
        using (RegistryKey key = Registry.LocalMachine.OpenSubKey(UninstallKey))
            if (key != null)
                vst3Folder = key.GetValue(InstallPathValue) as string;

        if (!String.IsNullOrEmpty(vst3Folder))
        {
            string bundle = Path.Combine(vst3Folder, "Transient Shaper.vst3");
            if (Directory.Exists(bundle))
                Directory.Delete(bundle, true);
        }

        Registry.LocalMachine.DeleteSubKeyTree(UninstallKey, false);
        string productFolder = ProductFolder;
        string self = Assembly.GetExecutingAssembly().Location;
        string cleanup = "/c ping 127.0.0.1 -n 3 >nul & del /f /q \"" + self + "\" & rmdir \"" + productFolder + "\"";
        Process.Start(new ProcessStartInfo("cmd.exe", cleanup) { CreateNoWindow = true, UseShellExecute = false, WindowStyle = ProcessWindowStyle.Hidden });
    }

    private sealed class SetupForm : Form
    {
        private readonly TextBox pathBox;
        private readonly Button installButton;
        private readonly Label statusLabel;

        internal SetupForm()
        {
            Text = "Transient Shaper VST3 Setup";
            ClientSize = new Size(540, 350);
            MinimumSize = MaximumSize = Size;
            StartPosition = FormStartPosition.CenterScreen;
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = true;
            Font = new Font("Segoe UI", 9F);
            BackColor = Color.White;

            var header = new Panel { Dock = DockStyle.Top, Height = 92, BackColor = Color.FromArgb(29, 47, 61) };
            var title = new Label { Text = "Transient Shaper", ForeColor = Color.White, Font = new Font("Segoe UI Semibold", 17F), AutoSize = true, Location = new Point(26, 17) };
            var subtitle = new Label { Text = "VST3 plug-in setup", ForeColor = Color.FromArgb(185, 205, 215), Font = new Font("Segoe UI", 9F), AutoSize = true, Location = new Point(29, 54) };
            header.Controls.Add(title);
            header.Controls.Add(subtitle);
            Controls.Add(header);

            var intro = new Label { Text = "Install the VST3 plug-in for your audio applications.", AutoSize = true, Location = new Point(28, 119), Font = new Font("Segoe UI", 10F) };
            Controls.Add(intro);

            var pathLabel = new Label { Text = "VST3 plug-in folder", AutoSize = true, Location = new Point(29, 166), Font = new Font("Segoe UI Semibold", 9F) };
            Controls.Add(pathLabel);
            pathBox = new TextBox { Text = DefaultVst3Folder, Location = new Point(29, 191), Size = new Size(384, 27), Font = new Font("Segoe UI", 9F) };
            Controls.Add(pathBox);
            var browse = new Button { Text = "Browse...", Location = new Point(422, 189), Size = new Size(88, 30), FlatStyle = FlatStyle.System };
            browse.Click += BrowseForFolder;
            Controls.Add(browse);

            statusLabel = new Label { Text = "This installs the VST3 plug-in only. Close your DAW before installing.", AutoSize = false, Location = new Point(29, 237), Size = new Size(480, 37), ForeColor = Color.FromArgb(90, 100, 108) };
            Controls.Add(statusLabel);

            var footer = new Panel { Dock = DockStyle.Bottom, Height = 60, BackColor = Color.FromArgb(244, 246, 248) };
            var cancel = new Button { Text = "Cancel", Size = new Size(88, 30), Location = new Point(330, 15), FlatStyle = FlatStyle.System, DialogResult = DialogResult.Cancel };
            installButton = new Button { Text = "Install", Size = new Size(105, 30), Location = new Point(420, 15), FlatStyle = FlatStyle.System, BackColor = Color.FromArgb(53, 190, 169) };
            installButton.Click += InstallClicked;
            footer.Controls.Add(cancel);
            footer.Controls.Add(installButton);
            Controls.Add(footer);
            AcceptButton = installButton;
            CancelButton = cancel;
        }

        private void BrowseForFolder(object sender, EventArgs e)
        {
            using (var dialog = new FolderBrowserDialog { Description = "Choose the VST3 folder scanned by your DAW", SelectedPath = pathBox.Text, ShowNewFolderButton = true })
                if (dialog.ShowDialog(this) == DialogResult.OK)
                    pathBox.Text = dialog.SelectedPath;
        }

        private void InstallClicked(object sender, EventArgs e)
        {
            string folder = pathBox.Text.Trim();
            if (folder.Length == 0)
            {
                MessageBox.Show(this, "Choose a VST3 folder before installing.", ProductName, MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }

            try
            {
                installButton.Enabled = false;
                statusLabel.Text = "Installing the VST3 plug-in...";
                Application.DoEvents();
                Install(folder);
                statusLabel.Text = "Installation completed.";
                MessageBox.Show(this, "Transient Shaper VST3 was installed. Restart your DAW or rescan its plug-ins.", ProductName, MessageBoxButtons.OK, MessageBoxIcon.Information);
                Close();
            }
            catch (Exception ex)
            {
                installButton.Enabled = true;
                statusLabel.Text = "Installation could not be completed.";
                MessageBox.Show(this, "Could not install the plug-in. Close your DAW and check that you have permission to write to the selected folder.\r\n\r\n" + ex.Message, ProductName, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
