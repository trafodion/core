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
using System.Windows.Forms;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaMonitoringUserControl
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
            this.components = new System.ComponentModel.Container();
            this.TrafodionPanel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._ConnectivityMainBodyPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._connectivityTopTabControl = new Trafodion.Manager.Framework.Controls.TrafodionTabControl();
            this.theTopPanel = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this.theTopPanelLowerLabel = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();
            this.Monitor_TrafodionToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.TrafodionPanel1.SuspendLayout();
            this._ConnectivityMainBodyPanel.SuspendLayout();
            this.theTopPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // TrafodionPanel1
            // 
            this.TrafodionPanel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.TrafodionPanel1.Controls.Add(this._ConnectivityMainBodyPanel);
            this.TrafodionPanel1.Controls.Add(this.theTopPanel);
            this.TrafodionPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TrafodionPanel1.Location = new System.Drawing.Point(0, 0);
            this.TrafodionPanel1.Name = "TrafodionPanel1";
            this.TrafodionPanel1.Size = new System.Drawing.Size(996, 693);
            this.TrafodionPanel1.TabIndex = 4;
            // 
            // _ConnectivityMainBodyPanel
            // 
            this._ConnectivityMainBodyPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this._ConnectivityMainBodyPanel.Controls.Add(this._connectivityTopTabControl);
            this._ConnectivityMainBodyPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            this._ConnectivityMainBodyPanel.Location = new System.Drawing.Point(0, 33);
            this._ConnectivityMainBodyPanel.Name = "_ConnectivityMainBodyPanel";
            this._ConnectivityMainBodyPanel.Size = new System.Drawing.Size(996, 660);
            this._ConnectivityMainBodyPanel.TabIndex = 1;
            // 
            // _connectivityTopTabControl
            // 
            this._connectivityTopTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._connectivityTopTabControl.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._connectivityTopTabControl.Location = new System.Drawing.Point(0, 0);
            this._connectivityTopTabControl.Multiline = true;
            this._connectivityTopTabControl.Name = "_connectivityTopTabControl";
            this._connectivityTopTabControl.Padding = new System.Drawing.Point(10, 5);
            this._connectivityTopTabControl.SelectedIndex = 0;
            this._connectivityTopTabControl.Size = new System.Drawing.Size(996, 660);
            this._connectivityTopTabControl.TabIndex = 0;
            this._connectivityTopTabControl.Visible = false;
            // 
            // theTopPanel
            // 
            this.theTopPanel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanel.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.theTopPanel.Controls.Add(this.theTopPanelLowerLabel);
            this.theTopPanel.Dock = System.Windows.Forms.DockStyle.Top;
            this.theTopPanel.Location = new System.Drawing.Point(0, 0);
            this.theTopPanel.Name = "theTopPanel";
            this.theTopPanel.Size = new System.Drawing.Size(996, 33);
            this.theTopPanel.TabIndex = 4;
            // 
            // theTopPanelLowerLabel
            // 
            this.theTopPanelLowerLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.theTopPanelLowerLabel.BackColor = System.Drawing.Color.WhiteSmoke;
            this.theTopPanelLowerLabel.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.theTopPanelLowerLabel.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.theTopPanelLowerLabel.Location = new System.Drawing.Point(5, 7);
            this.theTopPanelLowerLabel.Name = "theTopPanelLowerLabel";
            this.theTopPanelLowerLabel.ReadOnly = true;
            this.theTopPanelLowerLabel.Size = new System.Drawing.Size(984, 15);
            this.theTopPanelLowerLabel.TabIndex = 2;
            this.theTopPanelLowerLabel.Text = "<theTopPanelLowerLabel>";
            // 
            // ConnectivityAreaMonitoringUserControl
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TrafodionPanel1);
            this.Name = "ConnectivityAreaMonitoringUserControl";
            this.Size = new System.Drawing.Size(996, 693);
            this.TrafodionPanel1.ResumeLayout(false);
            this._ConnectivityMainBodyPanel.ResumeLayout(false);
            this.theTopPanel.ResumeLayout(false);
            this.theTopPanel.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private Trafodion.Manager.Framework.Controls.TrafodionPanel TrafodionPanel1;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel _ConnectivityMainBodyPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTabControl _connectivityTopTabControl;
        private Trafodion.Manager.Framework.Controls.TrafodionPanel theTopPanel;
        private Trafodion.Manager.Framework.Controls.TrafodionTextBox theTopPanelLowerLabel;
        private Trafodion.Manager.Framework.Controls.TrafodionToolTip Monitor_TrafodionToolTip;


    }
}
