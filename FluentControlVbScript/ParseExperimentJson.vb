Imports Microsoft.VisualBasic
Imports System
Imports System.IO
Imports Tecan.Core.Scripting

Public Class TestClass
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

    ' ===========================================================================
    ' ParseExperimentJson.vb
    '
    ' Reads Audit\experiment.json and extracts the first "requested_tests" value.
    '
    ' FluentControl variable READ:
    '   RootPath        (String)  e.g. Z:\Fluent\Experiments\EXP2025Test\
    '
    ' FluentControl variable WRITTEN:
    '   RequestedTest   (String)  e.g. INV-T-005
    ' ===========================================================================

    Public Sub Execute() Implements IScriptObject.Execute

        ' --- 1. Read RootPath ---
        Dim rootPath As String = Convert.ToString(Host.GetVariable("RootPath")).Trim()

        If rootPath = "" Then
            Throw New Exception("RootPath is empty.")
        End If

        ' --- 2. Build path to experiment.json ---
        Dim auditFolder As String = Path.Combine(rootPath, "Audit")
        Dim jsonPath As String = Path.Combine(auditFolder, "experiment.json")

        If Not File.Exists(jsonPath) Then
            Throw New Exception("experiment.json not found: " & jsonPath)
        End If

        ' --- 3. Read the file ---
        Dim jsonText As String = File.ReadAllText(jsonPath)

        If jsonText = "" Then
            Throw New Exception("experiment.json is empty: " & jsonPath)
        End If

        ' --- 4. Find the key ---
        Dim keyToken As String = Chr(34) & "requested_tests" & Chr(34)
        Dim keyPos As Integer = jsonText.IndexOf(keyToken)

        If keyPos < 0 Then
            Throw New Exception("Key requested_tests not found in: " & jsonPath)
        End If

        ' --- 5. Skip past the key, whitespace and colon ---
        Dim pos As Integer = keyPos + keyToken.Length

        Dim keepGoing As Boolean = True
        While pos < jsonText.Length And keepGoing
            Dim c As Char = jsonText(pos)
            If c = " "c Or c = ":"c Or c = Chr(9) Or c = Chr(13) Or c = Chr(10) Then
                pos = pos + 1
            Else
                keepGoing = False
            End If
        End While

        ' --- 6. Expect opening quote ---
        If pos >= jsonText.Length Or jsonText(pos) <> Chr(34) Then
            Throw New Exception("No string value found after requested_tests key.")
        End If

        pos = pos + 1

        ' --- 7. Read until closing quote ---
        Dim result As String = ""

        While pos < jsonText.Length
            Dim ch As Char = jsonText(pos)
            If ch = Chr(34) Then
                Exit While
            ElseIf ch = "\"c Then
                pos = pos + 1
                If pos < jsonText.Length Then
                    result = result & jsonText(pos)
                End If
            Else
                result = result & ch
            End If
            pos = pos + 1
        End While

        result = result.Trim()

        If result = "" Then
            Throw New Exception("requested_tests value is empty in: " & jsonPath)
        End If

        ' --- 8. Write to FluentControl ---
        Host.SetVariable("RequestedTest", result)

    End Sub

End Class