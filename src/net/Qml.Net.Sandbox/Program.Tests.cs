﻿using System;
using System.Collections.Generic;
using System.Threading;
using Qml.Net.Tests;
using Xunit;
using Xunit.Abstractions;

namespace Qml.Net.Sandbox
{
    class Sink : IMessageSink
    {
        public bool OnMessage(IMessageSinkMessage message)
        {
            switch (message)
            {
                case ITestAssemblyFinished _:
                    TestAssemblyFinished.Set();
                    break;
                case ITestPassed testPassed:
                    Console.WriteLine($"Passed: {testPassed.TestCase.DisplayName}");
                    break;
                case ITestFailed testFailed:
                    Console.WriteLine($"Failed: {testFailed.TestCase.DisplayName}");
                    break;
            }
            return true;
        }
        
        public readonly ManualResetEvent TestAssemblyFinished = new ManualResetEvent(false);
    }

    class SinkWithTypes : IMessageSinkWithTypes
    {
        public void Dispose()
        {
            
        }

        public bool OnMessageWithTypes(IMessageSinkMessage message, HashSet<string> messageTypes)
        {
            switch (message)
            {
                case IDiscoveryCompleteMessage _:
                    DiscoveryComplete.Set();
                    break;
                case ITestCaseDiscoveryMessage testCaseDiscoveryMessage:
                    if(testCaseDiscoveryMessage.TestCase.DisplayName.Contains("Can_raise_base_signal_from_qml_on_base_object_from_derived_object"))
                    TestCases.Add(testCaseDiscoveryMessage.TestCase);
                    break;
            }

            return true;
        }

        public readonly ManualResetEvent DiscoveryComplete = new ManualResetEvent(false);

        public readonly List<ITestCase> TestCases = new List<ITestCase>();
    }
    
    class Program
    {
        static void Main()
        {
            var config = ConfigReader.Load(typeof(BaseTests).Assembly.Location);
            var controller = new XunitFrontController(AppDomainSupport.Denied, typeof(BaseTests).Assembly.Location);
            var discoverOptions = TestFrameworkOptions.ForDiscovery(config);
            var executionOptions = TestFrameworkOptions.ForExecution(config);
            
            var sinkWithTypes = new SinkWithTypes();
            var sink = new Sink();
            
            // Discover the tests
            Console.WriteLine("Discovering...");
            controller.Find(false, sinkWithTypes, discoverOptions);
            sinkWithTypes.DiscoveryComplete.WaitOne();
            
            // Run the tests
            Console.WriteLine("Running...");
            controller.RunTests(sinkWithTypes.TestCases, sink, executionOptions);
            sink.TestAssemblyFinished.WaitOne();
        }
    }
}