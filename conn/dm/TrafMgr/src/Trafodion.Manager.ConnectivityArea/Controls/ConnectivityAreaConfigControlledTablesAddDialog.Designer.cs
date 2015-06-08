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
﻿using Trafodion.Manager.Framework.Controls;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
    partial class ConnectivityAreaConfigControlledTablesAddDialog
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.panel1 = new Trafodion.Manager.Framework.Controls.TrafodionPanel();
            this._theCancelButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theOKButton = new Trafodion.Manager.Framework.Controls.TrafodionButton();
            this._theToolTip = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this._theConnectivityAreaControlledTablesAddUserControl = new Trafodion.Manager.ConnectivityArea.Controls.ConnectivityAreaConfigControlledTablesAddUserControl();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // panel1
            // 
            this.panel1.AutoSize = true;
            this.panel1.BackColor = System.Drawing.Color.WhiteSmoke;
            this.panel1.Controls.Add(this._theCancelButton);
            this.panel1.Controls.Add(this._theOKButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 441);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(517, 34);
            this.panel1.TabIndex = 1;
            // 
            // _theCancelButton
            // 
            this._theCancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theCancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this._theCancelButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theCancelButton.Location = new System.Drawing.Point(440, 6);
            this._theCancelButton.Name = "_theCancelButton";
            this._theCancelButton.Size = new System.Drawing.Size(75, 23);
            this._theCancelButton.TabIndex = 2;
            this._theCancelButton.Text = "C&ancel";
            this._theCancelButton.UseVisualStyleBackColor = true;
            this._theCancelButton.Click += new System.EventHandler(this.TheCancelButtonClick);
            // 
            // _theOKButton
            // 
            this._theOKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this._theOKButton.Font = new System.Drawing.Font("Tahoma", 8.25F);
            this._theOKButton.Location = new System.Drawing.Point(359, 6);
            this._theOKButton.Name = "_theOKButton";
            this._theOKButton.Size = new System.Drawing.Size(75, 23);
            this._theOKButton.TabIndex = 1;
            this._theOKButton.Text = "O&K";
            this._theOKButton.UseVisualStyleBackColor = true;
            this._theOKButton.Click += new System.EventHandler(this.TheOKButtonClick);
            // 
            // _theToolTip
            // 
            this._theToolTip.IsBalloon = true;
            // 
            // _theConnectivityAreaControlledTablesAddUserControl
            // 
            this._theConnectivityAreaControlledTablesAddUserControl.AutoSize = true;
            this._theConnectivityAreaControlledTablesAddUserControl.BackColor = System.Drawing.Color.WhiteSmoke;
            this._theConnectivityAreaControlledTablesAddUserControl.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theConnectivityAreaControlledTablesAddUserControl.Location = new System.Drawing.Point(0, 0);
            this._theConnectivityAreaControlledTablesAddUserControl.Name = "_theConnectivityAreaControlledTablesAddUserControl";
            this._theConnectivityAreaControlledTablesAddUserControl.Size = new System.Drawing.Size(517, 475);
            this._theConnectivityAreaControlledTablesAddUserControl.TabIndex = 0;
            // 
            // ConnectivityAreaConfigControlledTablesAddDialog
            // 
            this.AcceptButton = this._theOKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this._theCancelButton;
            this.ClientSize = new System.Drawing.Size(517, 475);
            this.Controls.Add(this.panel1);
            this.Controls.Add(this._theConnectivityAreaControlledTablesAddUserControl);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "ConnectivityAreaConfigControlledTablesAddDialog";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.Text = "Trafodion Database Manager - Add Controlled Table";
            this.panel1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private ConnectivityAreaConfigControlledTablesAddUserControl _theConnectivityAreaControlledTablesAddUserControl;
        private TrafodionPanel panel1;
        private TrafodionButton _theCancelButton;
        private TrafodionButton _theOKButton;
        private TrafodionToolTip _theToolTip;
    }
}