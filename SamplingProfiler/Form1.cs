using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.Runtime.InteropServices;
using System.IO;
using Newtonsoft.Json;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.TrackBar;

namespace SamplingProfiler
{
    public partial class Form1 : Form
    {
        SampleData sampleData = new SampleData();
        byte[] mainMem;

        [DllImport("kernel32")]
        static extern bool AllocConsole();

        public Form1()
        {
            InitializeComponent();

#if DEBUG
            AllocConsole();
#else
            debugToolStripMenuItem.Visible = false;
#endif
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            FormAbout about = new FormAbout();
            about.ShowDialog();
        }

        private void addSampleDataToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (openFileDialogJson.ShowDialog() == DialogResult.OK)
            {
                sampleData.Append(openFileDialogJson.FileName);
                RenderSampleData(sampleData.Analyze());
            }
        }

        private void clearSampleDataToolStripMenuItem_Click(object sender, EventArgs e)
        {
            sampleData.Clear();
            listView1.Items.Clear();
        }

        private void loadMainMemoryDumpToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (openFileDialogBin.ShowDialog() == DialogResult.OK)
            {
                mainMem = File.ReadAllBytes(openFileDialogBin.FileName);
            }
        }

        private void dumpSamplesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (var sample in sampleData.samples)
            {
                Console.WriteLine("{0}: {1}", sample.time, sample.pc);
            }
        }

        private void RenderSampleData(SortedDictionary<UInt32, Segment> segments)
        {
            listView1.Items.Clear();

            foreach (var segment in segments)
            {
				if (segment.Value.hits == 1)
                    continue;

				TimeSpan time = TimeSpan.FromMilliseconds(segment.Value.timeTotal / 1000);

                ListViewItem item = new ListViewItem("0x" + segment.Key.ToString("X8"));
                item.SubItems.Add(time.ToString());
                item.SubItems.Add(segment.Value.hits.ToString());
                listView1.Items.Add(item);
            }
        }

        class OSTimeResult
        {
            public string[] reply;
        }

        class GetNearestNameReply
        {
            public string name;
            public int offset;
        }

        class GetNearestNameResult
        {
            public GetNearestNameReply reply;
        }



    }
}
