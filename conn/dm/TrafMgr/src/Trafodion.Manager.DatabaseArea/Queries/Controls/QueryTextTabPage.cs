//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
//

using System;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public class QueryTextTabPage : TrafodionTabPage
    {
        public QueryTextTabPage(string aTitle, string aText)
            : base(aTitle)
        {
            SqlStatementTextBox theTextBox = new SqlStatementTextBox();
            theTextBox.ReadOnly = true;
            theTextBox.Text = aText;
            theTextBox.Dock = DockStyle.Fill;
            Controls.Add(theTextBox);
        }

        public QueryTextTabPage(string aTitle, string aText, bool wordWrap)
            : base(aTitle)
        {
            SqlStatementTextBox theTextBox = new SqlStatementTextBox();
            theTextBox.ReadOnly = true;
            theTextBox.Text = aText;
            theTextBox.Dock = DockStyle.Fill;
            theTextBox.WordWrap = wordWrap;
            Controls.Add(theTextBox);
        }
    }
}
