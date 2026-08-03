Imports Microsoft.VisualBasic
Imports System
Imports System.Collections.Generic
Imports Tecan.Core.Scripting

Public Class CheckRequestedTestScript
    Implements IScriptObject

    Private Host As IScriptingHost

    Public Property ScriptingHost() As IScriptingHost Implements IScriptObject.ScriptingHost
        Get
            Return Host
        End Get
        Set(ByVal value As IScriptingHost)
            Host = value
        End Set
    End Property

    Public Sub Execute() Implements IScriptObject.Execute

        Try
            ' ------------------------------------------------------------------
            ' 1. Get the RequestedTest variable from FluentControl
            ' ------------------------------------------------------------------
            Dim requestedTest As String = Convert.ToString(Host.GetVariable("RequestedTest")).Trim()

            If String.IsNullOrEmpty(requestedTest) Then
                Throw New Exception("Variable 'RequestedTest' is empty or not set.")
            End If

            ' ------------------------------------------------------------------
            ' 2. Test-ID -> plate-format catalogue
            '    OrdinalIgnoreCase so "inv-t-031" matches "INV-T-031".
            ' ------------------------------------------------------------------
            Dim plateFormat As New Dictionary(Of String, String)(StringComparer.OrdinalIgnoreCase) From {
                {"INV-T-001", "96"},
                {"INV-T-004", "96"},
                {"INV-T-005", "384"},
                {"INV-T-006", "384"},
                {"INV-T-009", "96"},
                {"INV-T-010", "96"},
                {"INV-T-011", "96"},
                {"INV-T-012", "96"},
                {"INV-T-015", "96"},
                {"INV-T-016", "96"},
                {"INV-T-017", "96"},
                {"INV-T-018", "96"},
                {"INV-T-019", "96"},
                {"INV-T-020", "96"},
                {"INV-T-021", "96"},
                {"INV-T-022", "96"},
                {"INV-T-025", "96"},
                {"INV-T-027", "96"},
                {"INV-T-028", "96"},
                {"INV-T-029", "96"},
                {"INV-T-031", "96"},
                {"INV-T-032", "96"},
                {"INV-T-041", "96"},
                {"INV-T-042", "96"},
                {"INV-T-044", "384"},
                {"INV-T-045", "384"},
                {"INV-T-047", "384"},
                {"INV-T-049", "384"}
            }

            ' ------------------------------------------------------------------
            ' 3. Look up the format. Unknown ID -> halt loudly.
            ' ------------------------------------------------------------------
            Dim format As String = Nothing
            If Not plateFormat.TryGetValue(requestedTest, format) Then
                Throw New Exception("RequestedTest '" & requestedTest & "' is not in the catalogue. " & _
                                    "Check for a typo or add it to the plate-format list in this script.")
            End If

            ' ------------------------------------------------------------------
            ' 4. Set is96 (1 = 96-well test, 0 = 384-well test)
            '    Written explicitly in BOTH branches so a stale 1 from a
            '    previous loop iteration can never leak through.
            ' ------------------------------------------------------------------
            Dim is96Value As Integer
            If format = "96" Then
                is96Value = 1
            Else
                is96Value = 0
            End If
            Host.SetVariable("is96", is96Value)

            ' ------------------------------------------------------------------
            ' 5. Set isMTA (1 only for the specific MTA test INV-T-031)
            ' ------------------------------------------------------------------
            Dim isMtaValue As Integer
            If requestedTest.Equals("INV-T-031", StringComparison.OrdinalIgnoreCase) Then
                isMtaValue = 1
            Else
                isMtaValue = 0
            End If
            Host.SetVariable("isMTA", isMtaValue)

            ' Clear any previous error
            TrySetVar("ScriptError", "")

        Catch ex As Exception
            ' Pass the error back to the Fluent interface so it doesn't crash silently
            TrySetVar("ScriptError", ex.Message)
            Throw
        End Try

    End Sub

    ' ------------------------------------------------------------------
    ' Helper method to safely write back to Fluent variables
    ' ------------------------------------------------------------------
    Private Sub TrySetVar(ByVal name As String, ByVal value As Object)
        Try
            Host.SetVariable(name, value)
        Catch
            ' Ignore failures when writing the error variable
        End Try
    End Sub

End Class