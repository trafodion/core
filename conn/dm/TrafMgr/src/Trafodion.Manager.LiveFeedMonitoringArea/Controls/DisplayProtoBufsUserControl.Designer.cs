// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
﻿namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    partial class DisplayProtoBufsUserControl
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this._theTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.SuspendLayout();
            // 
            // _theTabControl
            // 
            this._theTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theTabControl.HotTrack = true;
            this._theTabControl.Location = new System.Drawing.Point(0, 0);
            this._theTabControl.Multiline = true;
            this._theTabControl.Name = "_theTabControl";
            this._theTabControl.Padding = new System.Drawing.Point(10, 5);
            this._theTabControl.SelectedIndex = 0;
            this._theTabControl.Size = new System.Drawing.Size(759, 396);
            this._theTabControl.TabIndex = 0;
            // 
            // DisplayProtoBufsUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this._theTabControl);
            this.Name = "DisplayProtoBufsUserControl";
            this.Size = new System.Drawing.Size(759, 396);
            this.ResumeLayout(false);

        }

        #endregion

        private Framework.Controls.TrafodionTabControl _theTabControl;
    }
}
