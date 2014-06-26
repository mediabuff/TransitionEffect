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

using AdvancedMediaSource;
using Windows.Media.Core;
using Windows.Media.MediaProperties;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkID=390556

namespace Transition_Effect
{
    
    public sealed partial class Playback : Page,INotifyPropertyChanged
    {
        DispatcherTimer PlaybackTimer = new DispatcherTimer();
        NavigationHelper navigationHelper;

		CAdvancedMediaSource ams;
		MediaStreamSource mss;
		VideoStreamDescriptor videoDesc;

        public Playback()
        {
			//CurrentVideo = "ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4";
			Unloaded += Playback_Unloaded;
			this.InitializeComponent();
			this.DataContext = this;

			PlaybackTimer.Interval = TimeSpan.FromMilliseconds(100);
			//PlaybackTimer.Tick += (ss, ee) =>
			//	{
			//		Position = Video.Position.TotalMilliseconds;
			//	};
			//PlaybackTimer.Start();

			navigationHelper = new NavigationHelper(this);

			ams = new CAdvancedMediaSource();

			if (!ams.IsInitialized())
			{
				int x = 0; //break on error
			}


			if (!ams.AddVideo("ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4", EIntroType.FadeIn, 5000, EOutroType.FadeOut, 5000, EVideoEffect.None))
			{
				int x = 0;
			}

			SVideoData vd;

			ams.GetVideoData(out vd);

			VideoEncodingProperties videoProperties = VideoEncodingProperties.CreateUncompressed(MediaEncodingSubtypes.Bgra8, vd.Width, vd.Height);
			videoDesc = new VideoStreamDescriptor(videoProperties);
			videoDesc.EncodingProperties.FrameRate.Numerator = vd.Numerator;
			videoDesc.EncodingProperties.FrameRate.Denominator = vd.Denominator;
			videoDesc.EncodingProperties.Bitrate = (uint)(((UInt64)vd.Numerator * vd.Width * vd.Height * 4) / vd.Denominator);

			if (vd.HasAudio)
			{
				AudioEncodingProperties audioProperties = AudioEncodingProperties.CreatePcm(vd.ASampleRate, vd.AChannelCount, vd.ABitsPerSample);
				AudioStreamDescriptor audioDesc = new AudioStreamDescriptor(audioProperties);

				mss = new MediaStreamSource(videoDesc, audioDesc);
			}
			else
			{
				mss = new MediaStreamSource(videoDesc);
			}

			TimeSpan spanBuffer = new TimeSpan(0, 0, 0, 0, 250);
			mss.BufferTime = spanBuffer;
			mss.Starting += MSS_Starting;
			mss.SampleRequested += MSS_SampleRequested;

			//Video.CurrentStateChanged += mediaPlayer_CurrentStateChanged;
			Video.SetMediaStreamSource(mss);
        }

		void MSS_Starting(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceStartingEventArgs args)
		{
			if (!ams.OnStart(videoDesc))
			{
				int x = 0;
			}

			args.Request.SetActualStartPosition(new TimeSpan(0));
		}

		void MSS_SampleRequested(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceSampleRequestedEventArgs args)
		{
			if (args.Request.StreamDescriptor is VideoStreamDescriptor)
			{
				ams.GenerateVideoSample(args.Request);
			}
			else if (args.Request.StreamDescriptor is AudioStreamDescriptor)
			{
				ams.GenerateAudioSample(args.Request);
			}
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
            //Maximum = Video.NaturalDuration.TimeSpan.TotalMilliseconds; 
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

		private void Slider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
		{
			RangeBase rb = sender as RangeBase;

			if(rb != null && ams != null)
			{
				ams.SetPlaybackRate((int)rb.Value, 10);
			}
		}
    }
}
