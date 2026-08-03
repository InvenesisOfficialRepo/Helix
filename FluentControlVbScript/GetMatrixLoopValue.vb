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
    ' GetMatrixLoopValue.vb
    '
    ' Opens the expected_matrix_barcodes.txt from the current daughter folder
    ' and counts how many barcodes are inside.
    '
    ' FluentControl variables READ:
    '   RootPath    (String)   e.g. Z:\Fluent\Experiments\EXP2025Test\
    '   HitPickDghtLoop    (Integer)  current daughter loop index, 1-based
    '
    ' FluentControl variable WRITTEN:
    '   MatrixLoopNb (Integer) number of barcodes found in the file
    ' ===========================================================================

    Public Sub Execute() Implements IScriptObject.Execute

        ' --- 1. Read RootPath ---
        Dim rootPath As String = Convert.ToString(Host.GetVariable("RootPath")).Trim()

        If rootPath = "" Then
            Throw New Exception("RootPath is empty.")
        End If

        ' --- 2. Read HitPickDghtLoop and build folder name (1-based → 0-based) ---
        Dim HitPickDghtLoop As Integer = Convert.ToInt32(Host.GetVariable("HitPickDghtLoop"))
        Dim dghtFolder As String = Path.Combine(rootPath, "dght_" & (HitPickDghtLoop - 1).ToString())

        If Not Directory.Exists(dghtFolder) Then
            Throw New Exception("Daughter folder not found: " & dghtFolder)
        End If

        ' --- 3. Build path to barcode list ---
        Dim filePath As String = Path.Combine(dghtFolder, "expected_matrix_barcodes.txt")

        If Not File.Exists(filePath) Then
            Throw New Exception("expected_matrix_barcodes.txt not found: " & filePath)
        End If

        ' --- 4. Count non-empty lines ---
        Dim lines() As String = File.ReadAllLines(filePath)
        Dim count As Integer = 0
        Dim i As Integer

        For i = 0 To lines.Length - 1
            If lines(i).Trim() <> "" Then
                count = count + 1
            End If
        Next

        If count = 0 Then
            Throw New Exception("No barcodes found in: " & filePath)
        End If

        ' --- 5. Write result to FluentControl ---
        Host.SetVariable("MatrixLoopNb", count)

    End Sub

End Class