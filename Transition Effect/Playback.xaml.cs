using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Transition_Effect.Common;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

namespace Transition_Effect
{
    
    public sealed partial class Playback : Page,INotifyPropertyChanged
    {
        DispatcherTimer PlaybackTimer = new DispatcherTimer();
        NavigationHelper navigationHelper;

        public Playback()
        {
            CurrentVideo = "ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4";
            Unloaded += Playback_Unloaded;
            this.InitializeComponent();
            this.DataContext = this;

            PlaybackTimer.Interval = TimeSpan.FromMilliseconds(100);
            PlaybackTimer.Tick += (ss, ee) =>
                {
                    Position = Video.Position.TotalMilliseconds;
                };
            PlaybackTimer.Start();

            navigationHelper = new NavigationHelper(this);
        }

        void Playback_Unloaded(object sender, RoutedEventArgs e)
        {
            PlaybackTimer.Stop();
        }

        private void Save_click(object sender, RoutedEventArgs e)
        {
            
        }

        public string CurrentVideo { get; set; }

        private double maximum;
        public double Maximum
        {
            get { return maximum; }
            set { maximum = value;
               OnPropertyChanged("Maximum");
            }
        }
        

        private double position;
        public double Position
        {
            get { return position; }
            set { position = value;
            OnPropertyChanged("Position");
            }
        }

        private double playbackRate;
        public double PlaybackRate
        {
            get { return playbackRate; }
            set { playbackRate = value;
            OnPropertyChanged("PlaybackRate");
            }
        }
        

        private void OnPropertyChanged(string p)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(p));
        }


        public event PropertyChangedEventHandler PropertyChanged;

        private void MediaOpened(object sender, RoutedEventArgs e)
        {
            Maximum = Video.NaturalDuration.TimeSpan.TotalMilliseconds; 
        }

       
        private void SeekPosition(object sender, RangeBaseValueChangedEventArgs e)
        {
            int SliderValue = (int)e.NewValue;
            TimeSpan ts = new TimeSpan(0, 0, 0, 0, SliderValue);
            Video.Position = ts;
        }


        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedFrom(e);
        }
    }
}
