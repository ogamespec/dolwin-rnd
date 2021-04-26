﻿// File utilities

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.IO;

namespace ManagedUi
{
    class FileUtils
    {

        public enum FileType
        {
            All = 1,
            Dvd,
            Map,
            Patch
        }

        /// <summary>
        /// get file size
        /// </summary>
        /// <param name="path"></param>
        /// <returns></returns>
        public static long FileSize (string path)
        {
            FileInfo info = new FileInfo(path);
            return info.Length;
        }

        /// <summary>
        /// open file dialog
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        public static string FileOpen (FileType type, Config config)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();

            string prevDir = Directory.GetCurrentDirectory();

            switch (type)
            {
                case FileType.All:
                    openFileDialog.Filter = "All Supported Files|*.dol;*.elf;*.bin;*.gcm;*.iso|GameCube Executable Files|*.dol;*.elf|Binary Files|*.bin|GameCube DVD Images|*.gcm;*.iso|All Files|*.*";
                    Directory.SetCurrentDirectory(config.Settings.LastDirAll);
                    break;
                case FileType.Dvd:
                    openFileDialog.Filter = "GameCube DVD Images|*.gcm;*.iso|All Files|*.*";
                    Directory.SetCurrentDirectory(config.Settings.LastDirDvd);
                    break;
                case FileType.Map:
                    openFileDialog.Filter = "Symbolic information files|*.map|All Files|*.*";
                    Directory.SetCurrentDirectory(config.Settings.LastDirMap);
                    break;
                case FileType.Patch:
                    openFileDialog.Filter = "Patch files|*.xmlpatch|All Files|*.*";
                    Directory.SetCurrentDirectory(config.Settings.LastDirPatch);
                    break;
            }

            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                Directory.SetCurrentDirectory(prevDir);
                return openFileDialog.FileName;
            }

            Directory.SetCurrentDirectory(prevDir);
            return null;
        }

        /// <summary>
        /// nice value of KB, MB or GB, for output
        /// </summary>
        /// <param name="size">File size in bytes</param>
        /// <returns>Formatted string</returns>
        public static string NiceSize(long size)
        {
            if (size < 1024)
            {
                return size.ToString() + " byte";
            }
            else if (size < 1024*1024)
            {
                long kbytes = size / 1024;
                return kbytes.ToString() + " KB";
            }
            else if (size < 1024*1024*1024)
            {
                long mbytes = size / 1024 / 1024;
                return mbytes.ToString() + " MB";
            }
            else
            {
                float gbytes = (float)size / 1024 / 1024 / 1024;
                return gbytes.ToString("F2") + " GB";
            }
        }

    }
}
